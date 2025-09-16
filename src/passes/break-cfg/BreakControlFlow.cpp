//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/jitter.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/Metadata.hpp"
#include "omvll/passes/break-cfg/BreakControlFlow.hpp"
#include "omvll/utils.hpp"

using namespace llvm;

namespace omvll {

static constexpr size_t AArch64InstSize = 4;
static constexpr size_t AArch64FunctionAlignment =
    0x20; // Maximum value according to AArch64Subtarget.

static constexpr std::array<std::array<uint8_t, AArch64InstSize>, 4>
    AArch64NopInsts = {{
        {0x1F, 0x20, 0x03, 0xD5}, // nop
        {0xE0, 0x03, 0x00, 0xAA}, // mov x0, x0
        {0x42, 0x00, 0xC2, 0x93}, // ror x2, x2, #0
        {0xF4, 0x03, 0x14, 0xAA}, // mov x20, x20
    }};

static const uint8_t AArch64AsmBreakingStub[] = {
    0x10, 0x00, 0x00, 0x10, // adr x1, #0x10
    0x20, 0x0C, 0x40, 0xF9, // ldr x0, [x1, #56] (rounded from 61)
    0x41, 0x00, 0x00, 0x58, // ldr x1, =0x10
    0x20, 0x02, 0x3F, 0xD6, // blr x1
    0x41, 0x00, 0x00, 0x58, // ldr x1, =0x30
    0x60, 0x06, 0x3F, 0xD6, // blr x3
    0xF1, 0xFF, 0xF2, 0xA2, // raw bytes
    0xF8, 0xFF, 0xE2, 0xC2  // raw bytes
};

static constexpr size_t ARMInstSize = 4;
static constexpr size_t ARMFunctionAlignment = 0x10;

static constexpr std::array<std::array<uint8_t, ARMInstSize>, 4> ARMNopInsts = {
    {
        {0x00, 0xF0, 0x20, 0xE3}, // nop
        {0xA0, 0xF0, 0x21, 0xE3}, // mov r0, r0
        {0x00, 0x20, 0x82, 0xE3}, // orr r2, r2, #0
        {0x02, 0x20, 0xA0, 0xE1}, // mov r2, r2
    }};

static const uint8_t ARMAsmBreakingStub[] = {
    0x01, 0xA1,             // add r1, pc, #4
    0x0F, 0x68,             // ldr r0, [r1, #0x0F]
    0x09, 0x49,             // ldr r1, =#<addr>
    0x08, 0x47,             // bx  r1
    0x11, 0x49,             // ldr r1, =#<addr>
    0x18, 0x47,             // bx  r3
    0xF0, 0xFF, 0xF0, 0xE7, // raw bytes
    0xFE, 0xFF, 0xFF, 0xE7, // raw bytes
    // Pad with NOPs
    0x00, 0xBF, 0x00, 0xBF, 0x00, 0xBF, 0x00, 0xBF, 0x00, 0xBF, 0x00, 0xBF};

bool BreakControlFlow::runOnFunction(Function &F) {
  if (F.getInstructionCount() == 0)
    return false;

  if (F.isVarArg())
    return false;

  const auto &TT = Triple(F.getParent()->getTargetTriple());
  if (!(TT.isAArch64() || TT.isARM() || TT.isThumb()))
    return false;

  SINFO("[{}] Visiting function {}", name(), F.getName());

  ValueToValueMapTy VMap;
  ClonedCodeInfo CCI;
  Function *ClonedF = CloneFunction(&F, VMap, &CCI);
  F.deleteBody();

  Function &Trampoline = F;
  const size_t InstSize = TT.isAArch64() ? AArch64InstSize : ARMInstSize;
  const size_t FunctionAlignment =
      TT.isAArch64() ? AArch64FunctionAlignment : ARMFunctionAlignment;
  const auto &NopInsts = TT.isAArch64() ? AArch64NopInsts : ARMNopInsts;

  // ARM and Thumb may use the same stub.
  const auto AsmBreakingStub =
      TT.isAArch64() ? AArch64AsmBreakingStub : ARMAsmBreakingStub;
  const auto AsmBreakingStubSize = TT.isAArch64()
                                       ? sizeof(AArch64AsmBreakingStub)
                                       : sizeof(ARMAsmBreakingStub);
  StringRef StubRef(reinterpret_cast<const char *>(AsmBreakingStub),
                    AsmBreakingStubSize);
  std::unique_ptr<MemoryBuffer> Insts = MemoryBuffer::getMemBufferCopy(StubRef);

  if (Insts->getBufferSize() % FunctionAlignment)
    fatalError(fmt::format("Bad alignment for the assembly block ({})",
                           Insts->getBufferSize()));

  Constant *Prologue = nullptr;
  auto *Int8Ty = Type::getInt8Ty(F.getContext());
  size_t PrologueSize = 0;

  if (F.hasPrologueData()) {
    if (auto *Data = dyn_cast<ConstantDataSequential>(F.getPrologueData())) {
      StringRef Raw = Data->getRawDataValues();
      StringRef BreakingBuffer = Insts->getBuffer();
      SmallVector<uint8_t> NewPrologue;

      NewPrologue.reserve(Raw.size() + Insts->getBufferSize());
      NewPrologue.append(BreakingBuffer.begin(), BreakingBuffer.end());
      NewPrologue.append(Raw.begin(), Raw.end());

      if (NewPrologue.size() % FunctionAlignment) {
        const size_t Align = alignTo(NewPrologue.size(), FunctionAlignment);
        const size_t Delta = Align - NewPrologue.size();
        if (Delta % 4)
          fatalError("Bad alignment");

        const size_t Num = Delta / InstSize;
        std::uniform_int_distribution<uint32_t> Dist(0, NopInsts.size() - 1);

        for (size_t Idx = 0; Idx < Num; ++Idx) {
          const auto &NopInst = NopInsts[Dist(*RNG)];
          NewPrologue.append(NopInst.begin(), NopInst.end());
        }
      }

      PrologueSize = NewPrologue.size();
      Prologue = ConstantDataVector::get(F.getContext(), NewPrologue);
      F.setPrologueData(nullptr);
    }
  } else {
    PrologueSize = Insts->getBufferSize();
    Prologue = ConstantDataVector::getRaw(Insts->getBuffer(),
                                          Insts->getBufferSize(), Int8Ty);
  }

  if (!Prologue)
    fatalError("Missing prologue in function " + demangle(F.getName().str()));

  SDEBUG("[{}][{}] Injecting breaking stub", name(), F.getName());

  ClonedF->setPrologueData(Prologue);
  ClonedF->setLinkage(GlobalValue::InternalLinkage);

  BasicBlock *Entry =
      BasicBlock::Create(Trampoline.getContext(), "Entry", &Trampoline);
  IRBuilder<NoFolder> IRB(Entry);

  SmallVector<Value *> Args;
  for (auto &Arg : F.args())
    Args.push_back(&Arg);

  AllocaInst *VarDst = IRB.CreateAlloca(IRB.getInt64Ty());
  AllocaInst *VarFAddr = IRB.CreateAlloca(IRB.getInt64Ty());

  IRB.CreateStore(IRB.CreatePtrToInt(ClonedF, IRB.getInt64Ty()), VarFAddr,
                  /* volatile */ true);

  LoadInst *LoadFAddr = IRB.CreateLoad(IRB.getInt64Ty(), VarFAddr);

  Value *OpaqueFAddr =
      IRB.CreateAdd(LoadFAddr, ConstantInt::get(IRB.getInt64Ty(), 0));
  if (auto *Op = dyn_cast<Instruction>(OpaqueFAddr))
    addMetadata(*Op, {MetaObf(OpaqueOp, 2LLU), MetaObf(OpaqueCst)});

  Value *DstAddr = IRB.CreateAdd(
      OpaqueFAddr, ConstantInt::get(IRB.getInt64Ty(), PrologueSize));
  if (auto *Op = dyn_cast<Instruction>(DstAddr))
    addMetadata(*Op, {MetaObf(OpaqueOp, 2LLU), MetaObf(OpaqueCst)});

  StoreInst *StoreFunc = IRB.CreateStore(DstAddr, VarDst, /* volatile */ true);
  addMetadata(*StoreFunc, {MetaObf(ProtectFieldAccess), MetaObf(OpaqueCst)});

  LoadInst *FAddr =
      IRB.CreateLoad(IRB.getInt64Ty(), VarDst, /* volatile */ true);

  // Cast integral address to ptr.
  FunctionType *FTy = ClonedF->getFunctionType();
  PointerType *FTyPtrTy = PointerType::getUnqual(FTy);

  Value *FuncPtr = IRB.CreateIntToPtr(FAddr, FTyPtrTy);
  CallInst *Call = IRB.CreateCall(FTy, FuncPtr, Args);

  // Copy ABI parameter attributes from the cloned function to its call-site.
  auto IsABIAttribute = [](Attribute::AttrKind K) {
    switch (K) {
    case Attribute::ByRef:
    case Attribute::ByVal:
    case Attribute::InAlloca:
    case Attribute::InReg:
    case Attribute::Preallocated:
    case Attribute::Returned:
    case Attribute::SExt:
    case Attribute::StackAlignment:
    case Attribute::StructRet:
    case Attribute::SwiftAsync:
    case Attribute::SwiftError:
    case Attribute::SwiftSelf:
    case Attribute::ZExt:
      return true;
    default:
      return false;
    }
  };

  for (unsigned ArgNo = 0; ArgNo < ClonedF->arg_size(); ++ArgNo)
    for (const Attribute &Attr : ClonedF->getAttributes().getParamAttrs(ArgNo))
      if (IsABIAttribute(Attr.getKindAsEnum()))
        Call->addParamAttr(ArgNo, Attr);

  if (FTy->getReturnType()->isVoidTy()) {
    IRB.CreateRetVoid();
  } else {
    IRB.CreateRet(Call);
  }

  // Clean conflicting attributes.
  Trampoline.removeFnAttr(Attribute::AlwaysInline);
  Trampoline.removeFnAttr(Attribute::InlineHint);
  Trampoline.removeFnAttr(Attribute::OptimizeNone);

  // Add required attributes.
  Trampoline.addFnAttr(Attribute::OptimizeForSize);
  Trampoline.addFnAttr(Attribute::NoInline);

  return true;
}

PreservedAnalyses BreakControlFlow::run(Module &M, ModuleAnalysisManager &FAM) {
  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  PyConfig &Config = PyConfig::instance();
  SINFO("[{}] Executing on module {}", name(), M.getName());

  std::vector<Function *> ToVisit;
  for (Function &F : M) {
    if (isFunctionGloballyExcluded(&F) || F.isDeclaration() ||
        F.isIntrinsic() || F.getName().starts_with("__omvll"))
      continue;

    if (Config.getUserConfig()->breakControlFlow(&M, &F))
      ToVisit.emplace_back(&F);
  }

  if (ToVisit.empty())
    return PreservedAnalyses::all();

  bool Changed = false;
  RNG = M.createRNG(name());
  JIT = std::make_unique<Jitter>(M.getTargetTriple());

  for (Function *F : ToVisit)
    Changed |= runOnFunction(*F);

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll

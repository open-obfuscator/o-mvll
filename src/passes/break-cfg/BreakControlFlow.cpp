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

static constexpr auto AArch64AsmBreakingStub = R"delim(
  // This block must be aligned on 32 bytes
  adr x1, #0x10;
  ldr x0, [x1, #61];
  ldr x1, #16;
  blr x1;
  ldr x1, #48;
  blr x3;
  .byte 0xF1, 0xFF, 0xF2, 0xA2;
  .byte 0xF8, 0xFF, 0xE2, 0xC2;
)delim";

static constexpr size_t ARMInstSize = 4;
static constexpr size_t ARMFunctionAlignment = 0x10;

static constexpr std::array<std::array<uint8_t, ARMInstSize>, 4> ARMNopInsts = {
    {
        {0x00, 0xF0, 0x20, 0xE3}, // nop
        {0xA0, 0xF0, 0x21, 0xE3}, // mov r0, r0
        {0x00, 0x20, 0x82, 0xE3}, // orr r2, r2, #0
        {0x02, 0x20, 0xA0, 0xE1}, // mov r2, r2
    }};

static constexpr auto ARMAsmBreakingStub = R"delim(
  // This block must be aligned on 16 bytes
  adr r1, #0x10;
  ldr r0, [r1, #61];
  ldr r1, #16;
  bl r1;
  ldr r1, #48;
  bl r3;
  .byte 0xF0, 0xFF, 0xF0, 0xE7;
  .byte 0xFE, 0xFF, 0xFF, 0xE7;
)delim";

bool BreakControlFlow::runOnFunction(Function &F) {
  if (F.getInstructionCount() == 0)
    return false;

  const auto &TT = Triple(F.getParent()->getTargetTriple());
  if (!(TT.isAArch64() || TT.isARM()))
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
  const auto &AsmBreakingStub =
      TT.isAArch64() ? AArch64AsmBreakingStub : ARMAsmBreakingStub;

  std::unique_ptr<MemoryBuffer> Insts = JIT->jitAsm(AsmBreakingStub, 8);
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

  // "Demote" StructRet arguments as it can introduces
  // a conflict in CodeGen (observed in CPython - _PyEval_InitGIL).
  for (const auto &Arg : ClonedF->args()) {
    if (Arg.hasStructRetAttr()) {
      unsigned ArgNo = Arg.getArgNo();
      ClonedF->removeParamAttr(ArgNo, Attribute::StructRet);
      ClonedF->addParamAttr(ArgNo, Attribute::NoAlias);
    }
  }

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
  Value *FPtr = IRB.CreatePointerCast(FAddr, ClonedF->getFunctionType());

  if (ClonedF->getFunctionType()->getReturnType()->isVoidTy()) {
    IRB.CreateCall(ClonedF->getFunctionType(), FPtr, Args);
    IRB.CreateRetVoid();
  } else {
    IRB.CreateRet(IRB.CreateCall(ClonedF->getFunctionType(), FPtr, Args));
  }

  Trampoline.addFnAttr(Attribute::OptimizeForSize);
  Trampoline.addFnAttr(Attribute::NoInline);

  return true;
}

PreservedAnalyses BreakControlFlow::run(Module &M, ModuleAnalysisManager &FAM) {
  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  PyConfig &Config = PyConfig::instance();
  SINFO("[{}] Executing on module {}", name(), M.getName());
  RNG = M.createRNG(name());
  JIT = std::make_unique<Jitter>(M.getTargetTriple());

  for (Function &F : M) {
    if (isFunctionGloballyExcluded(&F) || F.isDeclaration() || F.isIntrinsic())
      continue;

    if (Config.getUserConfig()->breakControlFlow(&M, &F))
      Changed |= runOnFunction(F);
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll

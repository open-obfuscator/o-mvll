#include <algorithm>

#include "omvll/Jitter.hpp"
#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/Metadata.hpp"
#include "omvll/passes/break-cfg/BreakControlFlow.hpp"
#include "omvll/utils.hpp"

#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/MC/SubtargetFeature.h>

#include <llvm/Support/RandomNumberGenerator.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace llvm;


namespace omvll {

static constexpr size_t INST_SIZE          = 4;
static constexpr size_t FUNCTION_ALIGNMENT = 0x20; // Maximum value according to AArch64Subtarget

static constexpr std::array<std::array<uint8_t, INST_SIZE>, 4> NOP_INSTS = {
  {
    {0x1f, 0x20, 0x03, 0xd5}, // NOP
    {0xe0, 0x03, 0x00, 0xaa}, // mov x0, x0
    {0x42, 0x00, 0xc2, 0x93}, // ror x2, x2, #0
    {0xf4, 0x03, 0x14, 0xaa}, // mov x20, x20
  }
};


bool BreakControlFlow::runOnFunction(Function &F) {

  if (F.getInstructionCount() == 0) {
    return false;
  }

  PyConfig& config = PyConfig::instance();
  if (!config.getUserConfig()->break_control_flow(F.getParent(), &F)) {
    return false;
  }


  SDEBUG("{}", F.getName().str());

  ValueToValueMapTy VMap;
  ClonedCodeInfo info;
  Function* FCopied = CloneFunction(&F, VMap, &info);
  std::vector<BasicBlock*> BBlocks;

  for (auto& BB : F) {
    BBlocks.push_back(&BB);
  }

  for (auto& BB : BBlocks) {
    BB->eraseFromParent();
  }

  Function& Trampoline = F;
  std::unique_ptr<MemoryBuffer> insts = Jitter_->jitAsm(R"delim(
  // !! This block must be aligned on 32 bytes !!
  adr x1, #0x10;
  ldr x0, [x1, #61];
  ldr x1, #16;
  blr x1;
  ldr x1, #48;
  blr x3;
  .byte 0xF1, 0xFF, 0xF2, 0xA2;
  .byte 0xF8, 0xFF, 0xE2, 0xC2;
  )delim", 8);

  if (insts->getBufferSize() % FUNCTION_ALIGNMENT) {
    fatalError(fmt::format("Bad alignment for the ASM block ({})", insts->getBufferSize()));
  }

  Constant* Prologue = nullptr;
  auto* Int8Ty = Type::getInt8Ty(F.getContext());
  size_t PrologueSize = 0;

  if (F.hasPrologueData()) {
    if (auto* Data = dyn_cast<ConstantDataSequential>(F.getPrologueData())) {
      StringRef raw = Data->getRawDataValues();
      StringRef breakingBuffer = insts->getBuffer();
      SmallVector<uint8_t> NewPrologue;
      NewPrologue.reserve(raw.size() + insts->getBufferSize());
      NewPrologue.append(breakingBuffer.begin(), breakingBuffer.end());
      NewPrologue.append(raw.begin(), raw.end());

      if (NewPrologue.size() % FUNCTION_ALIGNMENT) {
        const size_t align = alignTo(NewPrologue.size(), FUNCTION_ALIGNMENT);
        const size_t delta = align - NewPrologue.size();
        if (delta % 4) fatalError("Bad alignment");

        const size_t nb = delta / INST_SIZE;
        std::uniform_int_distribution<uint32_t> Dist(0, NOP_INSTS.size() - 1);

        for (size_t i = 0; i < nb; ++i) {
          auto& NOP = NOP_INSTS[Dist(*RNG_)];
          NewPrologue.append(NOP.begin(), NOP.end());
        }
      }
      PrologueSize = NewPrologue.size();
      Prologue = ConstantDataVector::get(F.getContext(), NewPrologue);
      F.setPrologueData(nullptr);
    }
  } else {
    PrologueSize = insts->getBufferSize();
    Prologue = ConstantDataVector::getRaw(insts->getBuffer(), insts->getBufferSize(), Int8Ty);
  }

  if (Prologue == nullptr) {
    fatalError("Can't inject BreakControlFlow prologue in the function "
               "'" + demangle(F.getName().str()) + "'");
  }

  FCopied->setPrologueData(Prologue);
  FCopied->setLinkage(GlobalValue::InternalLinkage);

  // "Demote" StructRet arguments as it can introduces
  // a conflict in CodeGen (observed in CPython - _PyEval_InitGIL)
  for (Argument& arg : FCopied->args()) {
    if (arg.hasStructRetAttr()) {
      unsigned ArgNo = arg.getArgNo();
      FCopied->removeParamAttr(ArgNo, Attribute::StructRet);
      FCopied->addParamAttr(ArgNo, Attribute::NoAlias);
    }
  }

  BasicBlock* Entry = BasicBlock::Create(Trampoline.getContext(), "Entry", &Trampoline);
  IRBuilder<NoFolder> IRB(Entry);

  SmallVector<Value*> args;
  for (Argument& A : F.args()) {
    args.push_back(&A);
  }
  AllocaInst* VarDst   = IRB.CreateAlloca(IRB.getInt64Ty());
  AllocaInst* VarFAddr = IRB.CreateAlloca(IRB.getInt64Ty());

  StoreInst* StoreFAddr = IRB.CreateStore(
      IRB.CreatePtrToInt(FCopied, IRB.getInt64Ty()), VarFAddr, /* volatile */true);

  LoadInst* LoadFAddr = IRB.CreateLoad(IRB.getInt64Ty(), VarFAddr);

  Value* OpaqueFAddr = IRB.CreateAdd(LoadFAddr,
                                     ConstantInt::get(IRB.getInt64Ty(), 0));
  if (auto* Op = dyn_cast<Instruction>(OpaqueFAddr)) {
    addMetadata(*Op, {MetaObf(OPAQUE_OP, 2llu),
                      MetaObf(OPAQUE_CST)});
  }

  Value* DstAddr = IRB.CreateAdd(OpaqueFAddr,
                                 ConstantInt::get(IRB.getInt64Ty(), PrologueSize));

  if (auto* Op = dyn_cast<Instruction>(DstAddr)) {
    addMetadata(*Op, {MetaObf(OPAQUE_OP, 2llu),
                      MetaObf(OPAQUE_CST)});
  }

  StoreInst* StoreFunc = IRB.CreateStore(DstAddr, VarDst, /* volatile */true);
  addMetadata(*StoreFunc, {MetaObf(PROTECT_FIELD_ACCESS),
                           MetaObf(OPAQUE_CST),});
  LoadInst* FAddr = IRB.CreateLoad(IRB.getInt64Ty(), VarDst, /* volatile */true);
  Value* FPtr = IRB.CreatePointerCast(FAddr, FCopied->getFunctionType());
  if (FCopied->getFunctionType()->getReturnType()->isVoidTy()) {
    IRB.CreateCall(FCopied->getFunctionType(), FPtr, args);
    IRB.CreateRetVoid();
  } else {
    IRB.CreateRet(IRB.CreateCall(FCopied->getFunctionType(), FPtr, args));
  }
  Trampoline.addFnAttr(Attribute::OptimizeForSize);
  Trampoline.addFnAttr(Attribute::NoInline);


  return true;
}



PreservedAnalyses BreakControlFlow::run(Module &M,
                                        ModuleAnalysisManager &FAM) {
  RNG_ = M.createRNG(name());
  Jitter_ = Jitter::Create(M.getTargetTriple());
  SINFO("[{}] Run on: {}", name(), M.getName().str());
  bool Changed = false;
  std::vector<Function*> Fs;
  for (Function& F : M) {
    Fs.push_back(&F);
  }

  for (Function* F : Fs) {
    Changed |= runOnFunction(*F);
  }

  SINFO("[{}] Done!", name());
  return Changed ? PreservedAnalyses::none() :
                   PreservedAnalyses::all();

}
}


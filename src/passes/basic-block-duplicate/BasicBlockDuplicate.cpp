//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/basic-block-duplicate/BasicBlockDuplicate.hpp"
#include "omvll/passes/basic-block-duplicate/BasicBlockDuplicateOpt.hpp"
#include "omvll/utils.hpp"

using namespace llvm;
using namespace omvll;

namespace omvll {

static constexpr auto CoinflipFunctionName = "__omvll_coinflip";
static constexpr auto LRand48FunctionName = "lrand48";

bool BasicBlockDuplicate::process(Function &F, LLVMContext &Ctx,
                                  unsigned Probability) {
  SmallVector<BasicBlock *, 64> ToDup;
  ToDup.reserve(F.size());

  // Collect basic blocks to be duplicated.
  auto ShouldDuplicate = [&]() { return (*RNG)() % 100U < Probability; };
  for (BasicBlock &BB : F) {
    if (&BB == &F.getEntryBlock())
      continue;
    if (isEHBlock(BB) || containsSwiftErrorAlloca(BB))
      continue;
    if (ShouldDuplicate())
      ToDup.push_back(&BB);
  }

  if (ToDup.empty())
    return false;

  IRBuilder<> Builder(Ctx);
  for (BasicBlock *BB : ToDup) {
    Instruction *SplitPt = BB->getFirstNonPHI();
    if (!SplitPt)
      continue;

    // Duplicate the basic block and remap its instruction operands via VMap.
    ValueToValueMapTy VMap;
    BasicBlock *OldBB = SplitBlock(BB, SplitPt);
    BasicBlock *NewBB = CloneBasicBlock(OldBB, VMap, ".clone", BB->getParent());
    SmallVector<BasicBlock *, 1> Blocks{NewBB};
    remapInstructionsInBlocks(Blocks, VMap);

    // Drop original basic block terminator.
    BB->getTerminator()->eraseFromParent();

    // Branch on __omvll_coinflip result.
    Builder.SetInsertPoint(BB);
    Value *Cond = Builder.CreateCall(CoinflipCallee, {});
    Builder.CreateCondBr(Cond, NewBB, OldBB);

    // Ensure existing PNs have an incoming entry for the newly-cloned basic
    // block.
    for (BasicBlock *Succ : successors(NewBB)) {
      for (PHINode &PN : Succ->phis()) {
        Value *Incoming = PN.getIncomingValueForBlock(OldBB);
        if (auto *IncomingI = dyn_cast_or_null<Instruction>(Incoming))
          if (Value *Mapped = VMap.lookup(IncomingI))
            Incoming = Mapped;
        PN.addIncoming(Incoming, NewBB);
      }
    }

    // TODO: Should also need a mapping cloned block to their successors, and
    // adjust the cloned block terminator (if its successors have been cloned
    // too).

    // Rebuild SSA form by inserting missing phi nodes at merge points.
    SmallVector<PHINode *, 8> NewPHIs;
    SSAUpdater Updater(&NewPHIs);
    for (Instruction &I : *OldBB) {
      Value *ClonedVal = VMap.lookup(&I);
      if (!ClonedVal)
        continue;

      bool HasOutsideUsers = llvm::any_of(I.users(), [&](User *U) {
        if (auto *I = dyn_cast<Instruction>(U)) {
          BasicBlock *BB = I->getParent();
          return BB != OldBB && BB != NewBB;
        }
        return false;
      });

      if (!HasOutsideUsers)
        continue;

      Updater.Initialize(I.getType(), I.getName());
      Updater.AddAvailableValue(OldBB, &I);
      Updater.AddAvailableValue(NewBB, ClonedVal);

      for (Use &U : make_early_inc_range(I.uses())) {
        auto *UserI = dyn_cast<Instruction>(U.getUser());
        if (!UserI)
          continue;

        BasicBlock *BB = UserI->getParent();
        if (!isa<PHINode>(UserI) && (BB == OldBB || BB == NewBB))
          continue;

        Updater.RewriteUse(U);
      }
    }
  }

  SDEBUG("[{}] Basic blocks duplicated: {}", name(), ToDup.size());
  return true;
}

// A per-module routine modeling a toss of a coin, selecting the basic block to
// jump to.
void BasicBlockDuplicate::initializeCoinflipRoutine(Module &M) {
  LLVMContext &Ctx = M.getContext();
  IRBuilder<> Builder(Ctx);

  // TODO: Should also be marked as speculatable. An alternative here would be
  // to branch on freeze poison, and let LLVM choose a fixed, arbitrary value.
  // At the very least, this should restrain some optimizations to occur.
  {
    AttributeList AL;
    AL = AL.addFnAttribute(Ctx, Attribute::NoUnwind);
    AL = AL.addFnAttribute(Ctx, Attribute::AlwaysInline);
    CoinflipCallee =
        M.getOrInsertFunction(CoinflipFunctionName, AL, Builder.getInt1Ty());
  }

  auto *CoinflipFunction = cast<Function>(CoinflipCallee.getCallee());
  CoinflipFunction->setLinkage(GlobalValue::LinkOnceODRLinkage);
  CoinflipFunction->setVisibility(GlobalValue::HiddenVisibility);

  BasicBlock *Entry = BasicBlock::Create(Ctx, "entry", CoinflipFunction);
  Builder.SetInsertPoint(Entry);

  AttributeList AL;
  AL = AL.addFnAttribute(Ctx, Attribute::NoUnwind);
  auto *I64Ty = Builder.getInt64Ty();
  FunctionCallee LRand48 =
      M.getOrInsertFunction(LRand48FunctionName, AL, I64Ty);

  auto *LRand64Call = Builder.CreateCall(LRand48);
  Value *LSB = Builder.CreateAnd(LRand64Call, ConstantInt::get(I64Ty, 1));
  Value *Cmp = Builder.CreateICmpNE(LSB, ConstantInt::get(I64Ty, 0));
  Builder.CreateRet(Cmp);
}

PreservedAnalyses BasicBlockDuplicate::run(Module &M,
                                           ModuleAnalysisManager &MAM) {
  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  PyConfig &Config = PyConfig::instance();
  LLVMContext &Ctx = M.getContext();
  RNG = M.createRNG(name());
  SINFO("[{}] Executing on module {}", name(), M.getName());

  BasicBlockDuplicateOpt Opt =
      Config.getUserConfig()->basicBlockDuplicate(&M, nullptr);
  if (std::get_if<BasicBlockDuplicateWithProbability>(&Opt))
    initializeCoinflipRoutine(M);

  for (Function &F : M) {
    Opt = Config.getUserConfig()->basicBlockDuplicate(&M, &F);

    if (isCoroutine(&F))
      continue;

    auto *P = std::get_if<BasicBlockDuplicateWithProbability>(&Opt);
    if (P && !isFunctionGloballyExcluded(&F) && !F.isDeclaration() &&
        !F.isIntrinsic() && !F.getName().starts_with("__omvll"))
      Changed |= process(F, Ctx, P->Probability);
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <cassert>
#include <random>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/indirect-branch/IndirectBranch.hpp"
#include "omvll/passes/indirect-branch/IndirectBranchOpt.hpp"
#include "omvll/utils.hpp"

using namespace llvm;
using namespace omvll;

namespace omvll {

bool IndirectBranch::process(Function &F, const DataLayout &DL,
                             LLVMContext &Ctx) {
  Module &M = *F.getParent();
  std::vector<Instruction *> TerminatorsToReplace;
  SmallPtrSet<BasicBlock *, 32> SuccBlocks;

  // Gather jumps and switch instructions.
  for (BasicBlock &BB : F) {
    auto *TI = BB.getTerminator();
    if (isa<BranchInst>(TI) || isa<SwitchInst>(TI)) {
      TerminatorsToReplace.emplace_back(TI);
      for (BasicBlock *Succ : successors(&BB))
        SuccBlocks.insert(Succ);
    }
  }

  if (TerminatorsToReplace.empty())
    return false;

  // Gather BBs addresses and shuffle them.
  std::vector<Constant *> ShuffledBlockAddrs;
  ShuffledBlockAddrs.reserve(SuccBlocks.size());
  for (BasicBlock *Succ : SuccBlocks)
    ShuffledBlockAddrs.emplace_back(BlockAddress::get(Succ));

  std::shuffle(ShuffledBlockAddrs.begin(), ShuffledBlockAddrs.end(),
               std::default_random_engine(Seed));

  // Build a jump table with the shuffled BBs addresses as targets.
  auto *ArrayTy =
      ArrayType::get(PointerType::getUnqual(Ctx), ShuffledBlockAddrs.size());
  auto *JumpTable = new GlobalVariable(
      M, ArrayTy, true, GlobalValue::PrivateLinkage,
      ConstantArray::get(ArrayTy, ShuffledBlockAddrs), "indbr.block_addresses");

  JumpTable->setAlignment(Align(8));
  JumpTable->setUnnamedAddr(GlobalValue::UnnamedAddr::None);
  if (Triple(M.getTargetTriple()).isiOS())
    JumpTable->setSection("__DATA,__const");

  DenseMap<BasicBlock *, unsigned> BlockToIdx;
  auto Cleanup = make_scope_exit([&]() {
    SuccBlocks.clear();
    BlockToIdx.clear();
  });

  for (auto [Idx, C] : enumerate(ShuffledBlockAddrs))
    BlockToIdx[cast<BlockAddress>(C)->getBasicBlock()] = Idx;

  unsigned Replaced = 0;
  IRBuilder<> Builder(Ctx);
  Type *IndexTy = DL.getIndexType(JumpTable->getType());
  auto *Zero = ConstantInt::get(IndexTy, 0);

  // Translate collected direct branches into indirect ones.
  for (Instruction *TI : TerminatorsToReplace) {
    // TODO: May be worth avoiding translating branches in critical edges (or
    // splitting them beforehand), as they may break some invariances to
    // subsequent code motion optimizations.
    // TODO: Add threshold on the maximum number of branches to convert?
    // TODO: Try to move this check above.
    if (TI->getParent()->size() == 1)
      continue;

    Builder.SetInsertPoint(TI);
    Value *LoadFrom = nullptr;

    auto CreateGEPForIdx = [&](BasicBlock *Dest) -> Value * {
      unsigned Idx = BlockToIdx[Dest];
      return Builder.CreateInBoundsGEP(JumpTable->getValueType(), JumpTable,
                                       {Zero, ConstantInt::get(IndexTy, Idx)});
    };

    if (auto *BI = dyn_cast<BranchInst>(TI)) {
      if (BI->isConditional())
        LoadFrom = Builder.CreateSelect(BI->getCondition(),
                                        CreateGEPForIdx(BI->getSuccessor(0)),
                                        CreateGEPForIdx(BI->getSuccessor(1)));
      else
        LoadFrom = CreateGEPForIdx(BI->getSuccessor(0));
    } else if (auto *SI = dyn_cast<SwitchInst>(TI)) {
      LoadFrom = CreateGEPForIdx(SI->getDefaultDest());

      for (const auto &Case : SI->cases()) {
        Value *Cond =
            Builder.CreateICmpEQ(SI->getCondition(), Case.getCaseValue());
        LoadFrom = Builder.CreateSelect(
            Cond, CreateGEPForIdx(Case.getCaseSuccessor()), LoadFrom);
      }
    }

    assert(LoadFrom && "Expecting a valid address value.");
    auto *IBI = Builder.CreateIndirectBr(
        Builder.CreateLoad(Builder.getPtrTy(), LoadFrom),
        TI->getNumSuccessors());

    for (auto *Succ : successors(TI->getParent()))
      IBI->addDestination(Succ);

    SDEBUG("[{}] Replacing direct branch {} with indirect ({})", name(),
           ToString(*TI), ToString(*IBI));
    TI->eraseFromParent();
    ++Replaced;
  }

  return Replaced > 0;
}

PreservedAnalyses IndirectBranch::run(Module &M, ModuleAnalysisManager &MAM) {
  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  PyConfig &Config = PyConfig::instance();
  Seed = RandomGenerator::generate();
  LLVMContext &Ctx = M.getContext();
  const auto &DL = M.getDataLayout();
  SINFO("[{}] Executing on module {}", name(), M.getName());

  for (Function &F : M) {
    IndirectBranchOpt Opt = Config.getUserConfig()->indirectBranch(&M, &F);
    if (!Opt || !*Opt)
      continue;

    if (isCoroutine(&F))
      continue;

    if (isFunctionGloballyExcluded(&F) ||
        F.hasFnAttribute(Attribute::AlwaysInline) || F.isDeclaration() ||
        F.isIntrinsic() || F.getName().starts_with("__omvll"))
      continue;

    Changed |= process(F, DL, Ctx);
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll

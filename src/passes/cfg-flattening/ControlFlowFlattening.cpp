//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/cfg-flattening/ControlFlowFlattening.hpp"
#include "omvll/utils.hpp"

using namespace llvm;

namespace omvll {

constexpr uint32_t Encode(uint32_t Id, uint32_t X, uint32_t Y) {
  return (Id ^ X) + Y;
}

template <class IRBTy>
void EmitTransition(IRBTy &IRB, AllocaInst *SV, BasicBlock *Dispatch,
                    uint32_t encId, uint32_t X, uint32_t Y) {
  IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), encId), SV);
  LoadInst *Val = IRB.CreateLoad(IRB.getInt32Ty(), SV, true);

  IRB.CreateStore(
      IRB.CreateAdd(IRB.CreateXor(Val, ConstantInt::get(IRB.getInt32Ty(), X)),
                    ConstantInt::get(IRB.getInt32Ty(), Y)),
      SV, true);

  IRB.CreateBr(Dispatch);
}

template <class IRBTy>
void EmitTransition(IRBTy &IRB, AllocaInst *SV, BasicBlock *Dispatch,
                    AllocaInst *TT, AllocaInst *TF, Value *Cond,
                    uint32_t TrueId, uint32_t FalseId, uint32_t X, uint32_t Y) {
  IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), TrueId), TT);
  IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), FalseId), TF);

  LoadInst *True = IRB.CreateLoad(IRB.getInt32Ty(), TT, true);
  LoadInst *False = IRB.CreateLoad(IRB.getInt32Ty(), TF, true);

  auto *EncTrue =
      IRB.CreateAdd(IRB.CreateXor(True, ConstantInt::get(IRB.getInt32Ty(), X)),
                    ConstantInt::get(IRB.getInt32Ty(), Y));

  auto *EncFalse =
      IRB.CreateAdd(IRB.CreateXor(False, ConstantInt::get(IRB.getInt32Ty(), X)),
                    ConstantInt::get(IRB.getInt32Ty(), Y));

  IRB.CreateStore(IRB.CreateSelect(Cond, EncTrue, EncFalse), SV, true);
  IRB.CreateBr(Dispatch);
}

template <class IRBTy> void EmitDefaultCaseAssembly(IRBTy &IRB, Triple TT) {
  // clang-format off
  auto *FType = FunctionType::get(IRB.getVoidTy(), false);
  if (TT.isAArch64()) {
    IRB.CreateCall(FType, InlineAsm::get(
      FType,
      R"delim(
        ldr x1, #-8;
        blr x1;
        mov x0, x1;
        .byte 0xF1, 0xFF;
        .byte 0xF2, 0xA2;
      )delim",
      "",
      /* hasSideEffects */ true,
      /* isStackAligned */ true
    ));
  } else if (TT.isX86()) {
    // FIXME: This assembly may not confuse a decompiler.
    IRB.CreateCall(FType, InlineAsm::get(
      FType,
      R"delim(
        nop;
        .byte 0xF1, 0xFF;
        .byte 0xF2, 0xA2;
      )delim",
      "",
      /* hasSideEffects */ true,
      /* isStackAligned */ true
    ));
  } else if (TT.isARM()) {
    IRB.CreateCall(FType, InlineAsm::get(
      FType,
      R"delim(
        ldr r1, [pc, #-4];
        bx r1;
        mov r1, r2;
        .byte 0xF1, 0xFF;
        .byte 0xF2, 0xA2;
      )delim",
      "",
      /* hasSideEffects */ true,
      /* isStackAligned */ true
    ));
  } else {
    fatalError("Unsupported target for Control-Flow Flattening obfuscation: " +
               TT.str());
  }
  // clang-format on
}

bool ControlFlowFlattening::runOnFunction(Function &F,
                                          RandomNumberGenerator &RNG) {
  if (F.getInstructionCount() == 0)
    return false;

  bool Changed = false;
  std::string DemangledName = demangle(F.getName().str());
  std::uniform_int_distribution<uint32_t> Dist(10);
  std::uniform_int_distribution<uint8_t> Dist8(10, 254);
  const uint8_t X = Dist8(RNG);
  const uint8_t Y = Dist8(RNG);

  SINFO("[{}] Visiting function {}", ControlFlowFlattening::name(),
        DemangledName);

  SmallVector<BasicBlock *, 20> FlattedBBs;
  demotePHINode(F);

  BasicBlock *EntryBlock = &F.getEntryBlock();
  DenseSet<BasicBlock *> NormalDest2Split;

  auto IsFromInvoke = [](const auto &BB) {
    return any_of(predecessors(&BB), [](const auto *Pred) {
      return isa<InvokeInst>(Pred->getTerminator());
    });
  };

  for (BasicBlock &BB : F) {
    if (BB.isLandingPad())
      continue;

    if (IsFromInvoke(BB))
      NormalDest2Split.insert(&BB);
  }

  /*
   * +------------------+  +------------------+
   * |                  |  |                  |
   * +--------+---------+  +---------+--------+
   * | Normal | Unwind  |  |         |        |
   * +---+----+---------+  +----+----+--------+
   *     |                      |
   * +---+----+            +----+----+
   * |        |            |         +--+ Intermediate Block
   * |        |            +---------+  |
   * |        |                         |
   * +--------+            +---------+<-+ Original Block
   * Normal Dst            |         |
   *                       |         |
   *                       |         |
   *                       +---------+
   */

  for (BasicBlock *BB : NormalDest2Split) {
    auto *Trampoline = BasicBlock::Create(BB->getContext(), ".normal_split",
                                          BB->getParent(), BB);
    for (BasicBlock *Pred : predecessors(BB)) {
      // Handle Invoke
      if (auto *Invoke = dyn_cast<InvokeInst>(Pred->getTerminator())) {
        Invoke->setNormalDest(Trampoline);
        continue;
      }

      // Handle Branch
      if (auto *Branch = dyn_cast<BranchInst>(Pred->getTerminator())) {
        for (size_t Idx = 0; Idx < Branch->getNumSuccessors(); ++Idx) {
          if (Branch->getSuccessor(Idx) == BB)
            Branch->setSuccessor(Idx, Trampoline);
        }
        continue;
      }

      // Handle Switch
      if (auto *Switch = dyn_cast<SwitchInst>(Pred->getTerminator())) {
        SwitchInst::CaseIt Begin = Switch->case_begin();
        SwitchInst::CaseIt End = Switch->case_end();
        for (auto It = Begin; It != End; ++It) {
          if (It->getCaseSuccessor() == BB)
            It->setSuccessor(Trampoline);
        }
        continue;
      }
    }

    // Branch the Trampoline to the original BasicBlock.
    BranchInst::Create(BB, Trampoline);
  }

  for (BasicBlock &BB : F) {
    if (EntryBlock == &BB)
      continue;

    FlattedBBs.push_back(&BB);
  }

  const size_t BlockSize =
      count_if(FlattedBBs, [](const auto *BB) { return !BB->isLandingPad(); });
  if (BlockSize <= 1) {
    SWARN("[{}] Block too small (#{}) to be flattened",
          ControlFlowFlattening::name(), FlattedBBs.size());
    return false;
  }

  if (auto *Branch = dyn_cast<BranchInst>(EntryBlock->getTerminator())) {
    if (Branch->isConditional()) {
      Value *Cond = Branch->getCondition();
      if (auto *Inst = dyn_cast<Instruction>(Cond)) {
        BasicBlock *EntrySplit =
            EntryBlock->splitBasicBlockBefore(Inst, "EntrySplit");
        FlattedBBs.insert(FlattedBBs.begin(), EntryBlock);

#ifdef OMVLL_DEBUG
        for (Instruction &I : *EntrySplit) {
          SDEBUG("[{}][EntrySplit] {}", ControlFlowFlattening::name(),
                 ToString(I));
        }

        for (Instruction &I : *EntryBlock) {
          SDEBUG("[{}][EntryBlock] {}", ControlFlowFlattening::name(),
                 ToString(I));
        }
#endif

        EntryBlock = EntrySplit;
      } else {
        SWARN("[{}] Found condition that is not an instruction",
              ControlFlowFlattening::name());
      }
    }
  } else if (auto *Switch = dyn_cast<SwitchInst>(EntryBlock->getTerminator())) {
    BasicBlock *EntrySplit =
        EntryBlock->splitBasicBlockBefore(Switch, "EntrySplit");
    FlattedBBs.insert(FlattedBBs.begin(), EntryBlock);
    EntryBlock = EntrySplit;
  } else if (auto *Invoke = dyn_cast<InvokeInst>(EntryBlock->getTerminator())) {
    BasicBlock *EntrySplit =
        EntryBlock->splitBasicBlockBefore(Invoke, "EntrySplit");
    FlattedBBs.insert(FlattedBBs.begin(), EntryBlock);
    EntryBlock = EntrySplit;
  }

  SDEBUG("[{}] Erasing {}", ControlFlowFlattening::name(),
         ToString(*EntryBlock->getTerminator()));
  EntryBlock->getTerminator()->eraseFromParent();

  // Create a state encoding for the BB to flatten.
  DenseMap<BasicBlock *, uint32_t> SwitchEnc;
  SmallSet<uint32_t, 20> SwitchRnd;
  for (BasicBlock *ToFlat : FlattedBBs) {
    if (ToFlat->isLandingPad())
      // Landing pads are not embedded in the switch.
      continue;

    uint32_t Rnd = 0;
    do {
      Rnd = Dist(RNG);
      uint32_t Enc = Encode(Rnd, X, Y);
      if (!SwitchRnd.contains(Rnd) && !SwitchRnd.contains(Enc)) {
        SwitchRnd.insert(Rnd);
        SwitchRnd.insert(Enc);
        break;
      }
    } while (true);
    SwitchEnc[ToFlat] = Rnd;
  }

  IRBuilder<> EntryIR(EntryBlock);
  AllocaInst *SwitchVar =
      EntryIR.CreateAlloca(EntryIR.getInt32Ty(), 0, "SwitchVar");
  AllocaInst *TmpTrue =
      EntryIR.CreateAlloca(EntryIR.getInt32Ty(), 0, "TmpTrue");
  AllocaInst *TmpFalse =
      EntryIR.CreateAlloca(EntryIR.getInt32Ty(), 0, "TmpFalse");

  EntryIR.CreateStore(EntryIR.getInt32(Encode(SwitchEnc[FlattedBBs[0]], X, Y)),
                      SwitchVar);

  auto &Ctx = F.getContext();
  auto *FlatLoopEntry =
      BasicBlock::Create(Ctx, "FlatLoopEntry", &F, EntryBlock);
  auto *FlatLoopEnd = BasicBlock::Create(Ctx, "FlatLoopEnd", &F, EntryBlock);
  auto *DefaultCase = BasicBlock::Create(Ctx, "DefaultCase", &F, FlatLoopEnd);

  IRBuilder<> FlatLoopEntryIR(FlatLoopEntry), FlatLoopEndIR(FlatLoopEnd),
      DefaultCaseIR(DefaultCase);

  LoadInst *LoadSwitchVar = FlatLoopEntryIR.CreateLoad(
      FlatLoopEntryIR.getInt32Ty(), SwitchVar, "SwitchVar");
  EntryBlock->moveBefore(FlatLoopEntry);
  EntryIR.CreateBr(FlatLoopEntry);
  FlatLoopEndIR.CreateBr(FlatLoopEntry);

  EmitDefaultCaseAssembly(DefaultCaseIR,
                          Triple(F.getParent()->getTargetTriple()));
  DefaultCaseIR.CreateBr(FlatLoopEnd);

  SwitchInst *Switch = FlatLoopEntryIR.CreateSwitch(LoadSwitchVar, DefaultCase);

  for (BasicBlock *ToFlat : FlattedBBs) {
    if (ToFlat->isLandingPad())
      // Landing pads should not be present in the switch case since they are
      // not directly called by flattened blocks.
      continue;

    auto ItEncId = SwitchEnc.find(ToFlat);
    if (ItEncId == SwitchEnc.end())
      fatalError(
          fmt::format("Cannot find the encoded index for the basic block: {}",
                      ToString(*ToFlat)));

    uint32_t SwitchId = Encode(ItEncId->second, X, Y);
    ToFlat->moveBefore(FlatLoopEnd);
    auto *Id = dyn_cast<ConstantInt>(
        ConstantInt::get(Switch->getCondition()->getType(), SwitchId));
    Switch->addCase(Id, ToFlat);
  }

  // Update the basic block with the switch var.
  for (BasicBlock *ToFlat : FlattedBBs) {
    Instruction *Term = ToFlat->getTerminator();
    SDEBUG("[{}] Flattening {} ({})", ControlFlowFlattening::name(),
           ToString(*ToFlat), ToString(*Term));

    if (isa<ReturnInst>(Term) || isa<UnreachableInst>(Term)) {
      /* Typically a ret instruction
       * {
       *  if (...) {
       *    return X;
       *  }
       * }
       *
       */
      continue;
    }

    if (isa<ResumeInst>(Term))
      // Nothing to do as it will 'resume' from information already known.
      continue;

    if (isa<InvokeInst>(Term))
      // Already processed with the early 'split'.
      continue;

    if (isa<SwitchInst>(Term)) {
      auto *SwitchTerm = dyn_cast<SwitchInst>(Term);

      std::vector<const SwitchInst::CaseHandle *> Cases;
      std::transform(
          SwitchTerm->case_begin(), SwitchTerm->case_end(),
          std::back_inserter(Cases),
          [](const SwitchInst::CaseHandle &Handle) { return &Handle; });

      for (const SwitchInst::CaseHandle &Handle : SwitchTerm->cases()) {
        BasicBlock *Target = Handle.getCaseSuccessor();
        auto ItEncId = SwitchEnc.find(Target);
        if (ItEncId == SwitchEnc.end())
          fatalError("Unable to find the encoded id for the basic block: " +
                     ToString(*Target));

        ConstantInt *DestId = Switch->findCaseDest(Target);
        if (!DestId)
          fatalError(fmt::format("Unable to find {} in the switch case",
                                 ToString(*Target)));

        const uint32_t EncId = ItEncId->second;
        auto *DispatchBlock = BasicBlock::Create(Ctx, "", &F, FlatLoopEnd);
        IRBuilder IRB(DispatchBlock);
        EmitTransition(IRB, SwitchVar, FlatLoopEnd, EncId, X, Y);
        SwitchTerm->setSuccessor(Handle.getSuccessorIndex(), DispatchBlock);
      }

      continue;
    }

    auto *Branch = dyn_cast<BranchInst>(Term);
    if (!Branch)
      fatalError(fmt::format("[{}] Weird '{}' is not a branch",
                             ControlFlowFlattening::name().str(),
                             ToString(*Term)));

    if (Branch->isUnconditional()) {
      BasicBlock *Target = Branch->getSuccessor(0);
      auto ItEncId = SwitchEnc.find(Target);
      if (ItEncId == SwitchEnc.end())
        fatalError(fmt::format(
            "Unable to find the encoded id for the basic block: '{}'",
            ToString(*Target)));

      ConstantInt *DestId = Switch->findCaseDest(Target);
      if (!DestId)
        fatalError(fmt::format("Unable to find {} in the switch case",
                               ToString(*Target)));

      const uint32_t EncId = ItEncId->second;
      IRBuilder IRB(Branch);
      EmitTransition(IRB, SwitchVar, FlatLoopEnd, EncId, X, Y);
      Branch->eraseFromParent();
      continue;
    }

    if (Branch->isConditional()) {
      BasicBlock *TrueCase = Branch->getSuccessor(0);
      BasicBlock *FalseCase = Branch->getSuccessor(1);

      auto ItTrue = SwitchEnc.find(TrueCase);
      auto ItFalse = SwitchEnc.find(FalseCase);

      if (ItTrue == SwitchEnc.end())
        fatalError(fmt::format(
            "Unable to find the encoded id for the (true) basic block: '{}'",
            ToString(*TrueCase)));

      if (ItFalse == SwitchEnc.end())
        fatalError(fmt::format(
            "Unable to find the encoded id for the (false) basic block: '{}'",
            ToString(*FalseCase)));

      ConstantInt *TrueId = Switch->findCaseDest(TrueCase);
      ConstantInt *FalseId = Switch->findCaseDest(FalseCase);

      if (!TrueId)
        fatalError(
            fmt::format("Unable to find {} (true case) in the switch case",
                        ToString(*TrueCase)));

      if (!FalseId)
        fatalError(
            fmt::format("Unable to find {} (false case) in the switch case",
                        ToString(*FalseCase)));

      const uint32_t TrueEncId = ItTrue->second;
      const uint32_t FalseEncId = ItFalse->second;

      IRBuilder IRB(Branch);
      EmitTransition(IRB, SwitchVar, FlatLoopEnd, TmpTrue, TmpFalse,
                     Branch->getCondition(), TrueEncId, FalseEncId, X, Y);
      Branch->eraseFromParent();
      continue;
    }
  }

  Changed = true;
  return Changed;
}

PreservedAnalyses ControlFlowFlattening::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  bool Changed = false;
  PyConfig &Config = PyConfig::instance();
  SINFO("[{}] Executing on module {}", name(), M.getName());
  std::unique_ptr<RandomNumberGenerator> RNG = M.createRNG(name());

  for (Function &F : M) {
    if (!Config.getUserConfig()->controlFlowGraphFlattening(&M, &F))
      continue;

    bool MadeChange = runOnFunction(F, *RNG);
    if (MadeChange)
      reg2mem(F);

    Changed |= MadeChange;
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll

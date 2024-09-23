#include "omvll/passes/flattening/ControlFlowFlattening.hpp"
#include "omvll/ObfuscationConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/utils.hpp"
#include "omvll/PyConfig.hpp"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"

using namespace llvm;

namespace omvll {

constexpr uint32_t Encode(uint32_t Id, uint32_t X, uint32_t Y) {
  return (Id ^ X) + Y;
}

template<class IRBTy>
void EmitTransition(IRBTy& IRB, AllocaInst* SV, BasicBlock* Dispatch,
                    uint32_t encId, uint32_t X, uint32_t Y) {
  IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), encId), SV);
  LoadInst* val = IRB.CreateLoad(IRB.getInt32Ty(), SV, true);

  IRB.CreateStore(
    IRB.CreateAdd(
        IRB.CreateXor(val, ConstantInt::get(IRB.getInt32Ty(), X)),
        ConstantInt::get(IRB.getInt32Ty(), Y)
    ), SV, true);

  IRB.CreateBr(Dispatch);
}

template<class IRBTy>
void EmitTransition(IRBTy& IRB, AllocaInst* SV, BasicBlock* Dispatch,
                    AllocaInst* TT, AllocaInst* TF, Value* Cond,
                    uint32_t TrueId, uint32_t FalseId, uint32_t X, uint32_t Y) {

  IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), TrueId), TT);
  IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), FalseId), TF);

  LoadInst* valTrue  = IRB.CreateLoad(IRB.getInt32Ty(), TT, true);
  LoadInst* valFalse = IRB.CreateLoad(IRB.getInt32Ty(), TF, true);

  auto* encTrue =
    IRB.CreateAdd(
        IRB.CreateXor(valTrue, ConstantInt::get(IRB.getInt32Ty(), X)),
        ConstantInt::get(IRB.getInt32Ty(), Y));

   auto* encFalse =
     IRB.CreateAdd(
        IRB.CreateXor(valFalse, ConstantInt::get(IRB.getInt32Ty(), X)),
        ConstantInt::get(IRB.getInt32Ty(), Y));

  IRB.CreateStore(
    IRB.CreateSelect(Cond, encTrue, encFalse),
    SV, true);
  IRB.CreateBr(Dispatch);
}


template<class IRBTy>
void EmitTransition(IRBTy& IRB, AllocaInst* V, BasicBlock* Dispatch,
                    uint32_t TrueId, uint32_t FalseId, uint32_t X, uint32_t Y) {

}

template<class IRBTy>
void EmitDefaultCaseAssembly(IRBTy& IRB, Triple TT) {
  auto* FType = FunctionType::get(IRB.getVoidTy(), false);
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
    // FIXME: This assembly may not confuse a decompiler
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
  } else {
    fatalError("Unsupported target for Control-Flow Flattening obfuscation: " + TT.str());
  }
}

bool ControlFlowFlattening::runOnFunction(Function& F, RandomNumberGenerator& RNG) {
  std::uniform_int_distribution<uint32_t> Dist(10);
  std::uniform_int_distribution<uint8_t> Dist8(10, 254);
  const uint8_t X = Dist8(RNG);
  const uint8_t Y = Dist8(RNG);

  StringRef name = F.getName();
  std::string demangledName = demangle(name.str());
  StringRef demangled = demangledName;

  bool Changed = false;

  if (F.getInstructionCount() == 0) {
    return false;
  }

  SINFO("[{}] Visiting function {}", ControlFlowFlattening::name(), demangled);

  SmallVector<BasicBlock*, 20> flattedBB;
  demotePHINode(F);

  BasicBlock* EntryBlock = &F.getEntryBlock();
  DenseSet<BasicBlock*> NormalDest2Split;

  auto IsFromInvoke = [] (const BasicBlock& BB) {
    return any_of(predecessors(&BB), [] (const BasicBlock* Pred) {
            return isa<InvokeInst>(Pred->getTerminator());
           });
  };

  for (BasicBlock& BB : F) {
    if (BB.isLandingPad()) {
      continue;
    }
    if (IsFromInvoke(BB)) {
      NormalDest2Split.insert(&BB);
    }
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


  for (BasicBlock* BB : NormalDest2Split) {
    auto* Trampoline = BasicBlock::Create(BB->getContext(), ".normal_split",
                                          BB->getParent(), BB);
    for (BasicBlock* Pred : predecessors(BB)) {
      // [Inst]: Invoke
      if (auto* Invoke = dyn_cast<InvokeInst>(Pred->getTerminator())) {
        Invoke->setNormalDest(Trampoline);
        continue;
      }
      // [Inst]: Br
      if (auto* Branch = dyn_cast<BranchInst>(Pred->getTerminator())) {
        for (size_t i = 0; i < Branch->getNumSuccessors(); ++i) {
          if (Branch->getSuccessor(i) == BB) {
            Branch->setSuccessor(i, Trampoline);
          }
        }
        continue;
      }
      // [Inst]: Switch
      if (auto* SW = dyn_cast<SwitchInst>(Pred->getTerminator())) {
        SwitchInst::CaseIt begin = SW->case_begin();
        SwitchInst::CaseIt end   = SW->case_end();
        for (auto it = begin; it != end; ++it) {
          if (it->getCaseSuccessor() == BB) {
            it->setSuccessor(Trampoline);
          }
        }
        continue;
      }
    }
    // Branch the Trampoline to the original BasicBlock
    BranchInst::Create(BB, Trampoline);
  }

  for (BasicBlock& BB : F) {
    if (&BB == EntryBlock) {
      continue;
    }
    flattedBB.push_back(&BB);
  }

  const size_t nbBlock = count_if(flattedBB, [] (BasicBlock* BB) {return !BB->isLandingPad();});
  if (nbBlock <= 1) {
    SWARN("[{}] Is too small (#{}) to be flattened",
          ControlFlowFlattening::name(), flattedBB.size());
    return false;
  }

  if (auto* br = dyn_cast<BranchInst>(EntryBlock->getTerminator())) {
    if (br->isConditional()) {
      Value* cond = br->getCondition();
      if (auto* instCond = dyn_cast<Instruction>(cond)) {
        BasicBlock *EntrySplit =
            EntryBlock->splitBasicBlockBefore(instCond, "EntrySplit");
        flattedBB.insert(flattedBB.begin(), EntryBlock);

#ifdef OMVLL_DEBUG
        for (Instruction &I : *EntrySplit) {
          SDEBUG("[{}][EntrySplit] {}", ControlFlowFlattening::name(),
                 ToString(I));
        }

        for (Instruction& I : *EntryBlock) {
          SDEBUG("[{}][EntryBlock] {}", ControlFlowFlattening::name(),
                 ToString(I));
        }
#endif // OMVLL_DEBUG

        EntryBlock = EntrySplit;
      } else {
        SWARN("[{}] Found condition is not an instruction",
              ControlFlowFlattening::name());
      }
    }
  }
  else if (auto* swInst = dyn_cast<SwitchInst>(EntryBlock->getTerminator())) {
    BasicBlock *EntrySplit =
        EntryBlock->splitBasicBlockBefore(swInst, "EntrySplit");
    flattedBB.insert(flattedBB.begin(), EntryBlock);
    EntryBlock = EntrySplit;
  }

  else if (auto* Invoke = dyn_cast<InvokeInst>(EntryBlock->getTerminator())) {
    BasicBlock *EntrySplit =
        EntryBlock->splitBasicBlockBefore(Invoke, "EntrySplit");
    flattedBB.insert(flattedBB.begin(), EntryBlock);
    EntryBlock = EntrySplit;
  }

  SDEBUG("[{}] Erasing {}", ControlFlowFlattening::name(),
         ToString(*EntryBlock->getTerminator()));
  EntryBlock->getTerminator()->eraseFromParent();

  /* Create a state encoding for the BB to flatten */
  DenseMap<BasicBlock*, uint32_t> SwitchEnc;
  SmallSet<uint32_t, 20> SwitchRnd;
  for (BasicBlock* toFlat : flattedBB) {
    if (toFlat->isLandingPad()) {
      /* LandingPad are not embedded in the switch */
      continue;
    }
    uint32_t rnd = 0;
    do {
      rnd = Dist(RNG);
      uint32_t enc = Encode(rnd, X, Y);
      if (!SwitchRnd.contains(rnd) && !SwitchRnd.contains(enc)) {
        SwitchRnd.insert(rnd);
        SwitchRnd.insert(enc);
        break;
      }
    } while (true);
    SwitchEnc[toFlat] = rnd;
  }

  IRBuilder<> EntryIR(EntryBlock);
  AllocaInst* switchVar = EntryIR.CreateAlloca(EntryIR.getInt32Ty(), 0, "SwitchVar");
  AllocaInst* tmpTrue   = EntryIR.CreateAlloca(EntryIR.getInt32Ty(), 0, "TmpTrue");
  AllocaInst* tmpFalse  = EntryIR.CreateAlloca(EntryIR.getInt32Ty(), 0, "TmpFalse");

  EntryIR.CreateStore(EntryIR.getInt32(Encode(SwitchEnc[flattedBB[0]], X, Y)), switchVar);

  auto* flatLoopEntry = BasicBlock::Create(F.getContext(), "FlatLoopEntry", &F, EntryBlock);
  auto* flatLoopEnd   = BasicBlock::Create(F.getContext(), "FlatLoopEnd", &F, EntryBlock);
  auto* defaultCase   = BasicBlock::Create(F.getContext(), "DefaultCase", &F, flatLoopEnd);

  IRBuilder<> FlatLoopEntryIR(flatLoopEntry),
              FlatLoopEndIR(flatLoopEnd),
              DefaultCaseIR(defaultCase);

  LoadInst* loadSwitchVar = FlatLoopEntryIR.CreateLoad(FlatLoopEntryIR.getInt32Ty(), switchVar, "SwitchVar");
  EntryBlock->moveBefore(flatLoopEntry);
  EntryIR.CreateBr(flatLoopEntry);
  FlatLoopEndIR.CreateBr(flatLoopEntry);

  EmitDefaultCaseAssembly(DefaultCaseIR, Triple(F.getParent()->getTargetTriple()));
  DefaultCaseIR.CreateBr(flatLoopEnd);

  SwitchInst* switchInst = FlatLoopEntryIR.CreateSwitch(loadSwitchVar, defaultCase);

  for (BasicBlock* toFlat : flattedBB) {
    if (toFlat->isLandingPad()) {
      /* LandingPad should not be present in the switch case
       * since they are not directly called by flattened blocks
       */
      continue;
    }
    auto itEncId = SwitchEnc.find(toFlat);
    if (itEncId == SwitchEnc.end()) {
      fatalError(fmt::format("Can't find the encoded index for the basic block: {}", ToString(*toFlat)));
    }
    uint32_t switchId = Encode(itEncId->second, X, Y);
    toFlat->moveBefore(flatLoopEnd);
    auto* id = dyn_cast<ConstantInt>(ConstantInt::get(switchInst->getCondition()->getType(), switchId));
    switchInst->addCase(id, toFlat);
  }

  /* Update the basic block with the switch var */
  for (BasicBlock* toFlat : flattedBB) {
    Instruction* terminator = toFlat->getTerminator();
    SDEBUG("[{}] Flattening {} ({})", ControlFlowFlattening::name(),
           ToString(*toFlat), ToString(*terminator));

    if (isa<ReturnInst>(terminator) || isa<UnreachableInst>(terminator)) {
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

    if (isa<ResumeInst>(terminator)) {
      /* Nothing to do as it will 'resume' from information
       * already known.
       */
      continue;
    }

    if (isa<InvokeInst>(terminator)) {
      /* Already processed with the early 'split' */
      continue;
    }

    if (isa<SwitchInst>(terminator)) {
      auto* switchTerm = dyn_cast<SwitchInst>(terminator);

      std::vector<const SwitchInst::CaseHandle*> cases;
      std::transform(switchTerm->case_begin(), switchTerm->case_end(), std::back_inserter(cases),
                     [] (const SwitchInst::CaseHandle& handle) { return &handle; });

      for (const SwitchInst::CaseHandle& handle : switchTerm->cases()) {
        BasicBlock* target = handle.getCaseSuccessor();
        auto itEncId = SwitchEnc.find(target);

        if (itEncId == SwitchEnc.end()) {
          fatalError("Unable to find the encoded id for the basic block: " + ToString(*target));
        }

        ConstantInt *destId = switchInst->findCaseDest(target);
        if (destId == nullptr) {
          fatalError(fmt::format("Unable to find {} in the switch case", ToString(*target)));
        }

        const uint32_t encId = itEncId->second;
        auto* dispatchBlock = BasicBlock::Create(F.getContext(), "", &F, flatLoopEnd);

        IRBuilder IRB(dispatchBlock);
        EmitTransition(IRB, switchVar, flatLoopEnd, encId, X, Y);
        switchTerm->setSuccessor(handle.getSuccessorIndex(), dispatchBlock);
      }
      continue;
    }

    auto* branch = dyn_cast<BranchInst>(terminator);
    if (branch == nullptr) {
      fatalError(fmt::format("[{}] Weird '{}' is not a branch",
                             ControlFlowFlattening::name().str(), ToString(*terminator)));
    }

    if (branch->isUnconditional()) {
      BasicBlock* target = branch->getSuccessor(0);
      auto itEncId = SwitchEnc.find(target);

      if (itEncId == SwitchEnc.end()) {
        fatalError(fmt::format("Unable to find the encoded id for the basic block: '{}'", ToString(*target)));
      }

      ConstantInt *destId = switchInst->findCaseDest(target);
      if (destId == nullptr) {
        fatalError(fmt::format("Unable to find {} in the switch case", ToString(*target)));
      }

      const uint32_t encId = itEncId->second;

      IRBuilder IRB(branch);
      EmitTransition(IRB, switchVar, flatLoopEnd, encId, X, Y);
      branch->eraseFromParent();
      continue;
    }

    if (branch->isConditional()) {
      BasicBlock* trueCase = branch->getSuccessor(0);
      BasicBlock* falseCase = branch->getSuccessor(1);

      auto itTrue = SwitchEnc.find(trueCase);
      auto itFalse = SwitchEnc.find(falseCase);

      if (itTrue == SwitchEnc.end()) {
        fatalError(fmt::format("Unable to find the encoded id for the (true) basic block: '{}'", ToString(*trueCase)));
      }

      if (itFalse == SwitchEnc.end()) {
        fatalError(fmt::format("Unable to find the encoded id for the (false) basic block: '{}'", ToString(*falseCase)));
      }

      ConstantInt *trueId  = switchInst->findCaseDest(trueCase);
      ConstantInt *falseId = switchInst->findCaseDest(falseCase);

      if (trueId == nullptr ) {
        fatalError(fmt::format("Unable to find {} (true case) in the switch case", ToString(*trueCase)));
      }
      if (falseId == nullptr ) {
        fatalError(fmt::format("Unable to find {} (false case) in the switch case", ToString(*falseCase)));
      }

      const uint32_t trueEncId  = itTrue->second;
      const uint32_t falseEncId = itFalse->second;

      IRBuilder IRB(branch);
      EmitTransition(IRB, switchVar, flatLoopEnd,
                     tmpTrue, tmpFalse, branch->getCondition(),
                     trueEncId, falseEncId, X, Y);
      branch->eraseFromParent();
      continue;
    }
  }

  Changed = true;
  return Changed;

}

PreservedAnalyses ControlFlowFlattening::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  PyConfig &config = PyConfig::instance();
  SINFO("[{}] Executing on module {}", name(), M.getName());
  std::unique_ptr<RandomNumberGenerator> RNG = M.createRNG(name());

  bool Changed = false;
  for (Function& F : M) {
    if (!config.getUserConfig()->flatten_cfg(&M, &F))
      continue;

    bool fChanged = runOnFunction(F, *RNG);

    if (fChanged) {
      reg2mem(F);
    }

    Changed |= fChanged;
  }

  SINFO("[{}] Changes{}applied on module {}", name(), Changed ? " " : " not ",
        M.getName());

  return Changed ? PreservedAnalyses::none() :
                   PreservedAnalyses::all();

}
}


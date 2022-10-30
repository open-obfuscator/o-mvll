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

  PyConfig& config = PyConfig::instance();
  if (!config.getUserConfig()->flatten_cfg(F.getParent(), &F)) {
    return false;
  }
  SINFO("Running CFG Flat on {}", demangled);

  SmallVector<BasicBlock*, 20> flattedBB;
  demotePHINode(F);

  BasicBlock* EntryBlock = &F.getEntryBlock();

  for (BasicBlock& BB : F) {
    if (&BB == EntryBlock) {
      continue;
    }
    flattedBB.push_back(&BB);
  }

  if (flattedBB.size() <= 1) {
    SWARN("[{}] Is too small (#{}) to be flattened", ControlFlowFlattening::name().str(), flattedBB.size());
    return false;
  }

  //if constexpr (is_debug) {
  //  SDEBUG("EntryBlock of {}: {}", demangled, ToString(F.getEntryBlock()));

  //  for (auto& BB : F) {
  //    SINFO("[BB] {}", ToString(BB));
  //    for (auto& I : BB) {
  //      SINFO("   {}", ToString(I));
  //    }
  //  }
  //}

  if (auto* br = dyn_cast<BranchInst>(EntryBlock->getTerminator())) {
    if (br->isConditional()) {
      Value* cond = br->getCondition();
      if (auto* instCond = dyn_cast<Instruction>(cond)) {
        BasicBlock* EntrySplited = EntryBlock->splitBasicBlockBefore(instCond, "EntrySplit");
        flattedBB.insert(flattedBB.begin(), EntryBlock);

        for (Instruction& I : *EntrySplited) {
          SINFO("[EntrySplited] {}", ToString(I));
        }

        for (Instruction& I : *EntryBlock) {
          SINFO("[EntryBlock  ] {}", ToString(I));
        }
        EntryBlock = EntrySplited;
      } else {
        SWARN("The condition is not an instruction");
      }
    }
  }

  if (auto* swInst = dyn_cast<SwitchInst>(EntryBlock->getTerminator())) {
    BasicBlock* EntrySplited = EntryBlock->splitBasicBlockBefore(swInst, "EntrySplit");
    flattedBB.insert(flattedBB.begin(), EntryBlock);
    EntryBlock = EntrySplited;
  }

  if (isa<InvokeInst>(EntryBlock->getTerminator())) {
    SINFO("{} is a single BB with a tail call", demangled);
    return false;
  }

  SINFO("Erasing {}", ToString(*EntryBlock->getTerminator()));
  EntryBlock->getTerminator()->eraseFromParent();

  /* Create a state encoding for the BB to flatten */
  DenseMap<BasicBlock*, uint32_t> SwitchEnc;
  SmallSet<uint32_t, 20> SwitchRnd;
  for (BasicBlock* toFlat : flattedBB) {
    uint32_t rnd = 0;
    do {
      rnd = Dist(RNG);
      uint32_t enc = (rnd ^ X) + Y;
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

  EntryIR.CreateStore(EntryIR.getInt32((SwitchEnc[flattedBB[0]] ^ X) + Y), switchVar);

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
  auto* FType = FunctionType::get(DefaultCaseIR.getVoidTy(), false);
  Value* rawAsm = InlineAsm::get(
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
  );
  DefaultCaseIR.CreateCall(FType, rawAsm);
  DefaultCaseIR.CreateBr(flatLoopEnd);

  SwitchInst* switchInst = FlatLoopEntryIR.CreateSwitch(loadSwitchVar, defaultCase);

  for (BasicBlock* toFlat : flattedBB) {
    auto itEncId = SwitchEnc.find(toFlat);
    if (itEncId == SwitchEnc.end()) {
      SERR("Can't find the encoded index for the basic block: {}", ToString(*toFlat));
      std::abort();
    }
    uint32_t switchId = (itEncId->second ^ X) + Y;
    toFlat->moveBefore(flatLoopEnd);
    auto* id = dyn_cast<ConstantInt>(ConstantInt::get(switchInst->getCondition()->getType(), switchId));
    switchInst->addCase(id, toFlat);
  }


  /* Update the basic block with the switch var */
  for (BasicBlock* toFlat : flattedBB) {
    Instruction* terminator = toFlat->getTerminator();
    SDEBUG("Flattening {} ({})", ToString(*toFlat), ToString(*terminator));

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
    if (isa<InvokeInst>(terminator) || isa<ResumeInst>(terminator)) {
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
          SWARN("Unable to find the encoded id for the basic block: '{}'", ToString(*target));
          std::abort();
        }

        ConstantInt *destId = switchInst->findCaseDest(target);
        if (destId == nullptr) {
          SWARN("Unable to find {} in the switch case", ToString(*target));
          std::abort();
          continue;
        }

        const uint32_t encId = itEncId->second;
        auto* dispatchBlock = BasicBlock::Create(F.getContext(), "", &F, flatLoopEnd);

        IRBuilder IRB(dispatchBlock);

        IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), encId), switchVar);
        LoadInst* val = IRB.CreateLoad(IRB.getInt32Ty(), switchVar, true);

        IRB.CreateStore(
          IRB.CreateAdd(
              IRB.CreateXor(val, ConstantInt::get(IRB.getInt32Ty(), X)),
              ConstantInt::get(IRB.getInt32Ty(), Y)
          ),
          switchVar, true);

        IRB.CreateBr(flatLoopEnd);
        switchTerm->setSuccessor(handle.getSuccessorIndex(), dispatchBlock);
      }
      continue;
    }

    auto* branch = dyn_cast<BranchInst>(terminator);
    if (branch == nullptr) {
      SWARN("[{}] Weird '{}' is not a branch", ControlFlowFlattening::name().str(), ToString(*terminator));
      std::abort();
    }

    if (branch->isUnconditional()) {
      BasicBlock* target = branch->getSuccessor(0);
      auto itEncId = SwitchEnc.find(target);

      if (itEncId == SwitchEnc.end()) {
        SWARN("Unable to find the encoded id for the basic block: '{}'", ToString(*target));
        std::abort();
      }

      ConstantInt *destId = switchInst->findCaseDest(target);
      if (destId == nullptr) {
        SWARN("Unable to find {} in the switch case", ToString(*target));
        std::abort();
        continue;
      }

      const uint32_t encId = itEncId->second;

      IRBuilder IRB(branch);

      IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), encId), switchVar);
      LoadInst* val = IRB.CreateLoad(IRB.getInt32Ty(), switchVar, true);

      IRB.CreateStore(
        IRB.CreateAdd(
            IRB.CreateXor(val, ConstantInt::get(IRB.getInt32Ty(), X)),
            ConstantInt::get(IRB.getInt32Ty(), Y)
        ),
        switchVar, true);

      IRB.CreateBr(flatLoopEnd);

      branch->eraseFromParent();
      continue;
    }

    if (branch->isConditional()) {
      BasicBlock* trueCase = branch->getSuccessor(0);
      BasicBlock* falseCase = branch->getSuccessor(1);

      auto itTrue = SwitchEnc.find(trueCase);
      auto itFalse = SwitchEnc.find(falseCase);

      if (itTrue == SwitchEnc.end()) {
        SWARN("Unable to find the encoded id for the (true) basic block: '{}'", ToString(*trueCase));
        std::abort();
      }

      if (itFalse == SwitchEnc.end()) {
        SWARN("Unable to find the encoded id for the (false) basic block: '{}'", ToString(*falseCase));
        std::abort();
      }

      ConstantInt *trueId  = switchInst->findCaseDest(trueCase);
      ConstantInt *falseId = switchInst->findCaseDest(falseCase);

      if (trueId == nullptr ) {
        SWARN("Unable to find {} (true case) in the switch case", ToString(*trueCase));
        std::abort();
      }
      if (falseId == nullptr ) {
        SWARN("Unable to find {} (false case) in the switch case", ToString(*falseCase));
        std::abort();
      }

      const uint32_t trueEncId  = itTrue->second;
      const uint32_t falseEncId = itFalse->second;

      IRBuilder IRB(branch);

      IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), trueEncId), tmpTrue);
      IRB.CreateStore(ConstantInt::get(IRB.getInt32Ty(), falseEncId), tmpFalse);

      LoadInst* valTrue = IRB.CreateLoad(IRB.getInt32Ty(), tmpTrue, true);
      LoadInst* valFalse = IRB.CreateLoad(IRB.getInt32Ty(), tmpFalse, true);

      auto* encTrue =
        IRB.CreateAdd(
            IRB.CreateXor(valTrue, ConstantInt::get(IRB.getInt32Ty(), X)),
            ConstantInt::get(IRB.getInt32Ty(), Y));

       auto* encFalse =
         IRB.CreateAdd(
            IRB.CreateXor(valFalse, ConstantInt::get(IRB.getInt32Ty(), X)),
            ConstantInt::get(IRB.getInt32Ty(), Y));

      IRB.CreateStore(
        IRB.CreateSelect(branch->getCondition(),
                         encTrue, encFalse),
        switchVar, true);
      IRB.CreateBr(flatLoopEnd);

      branch->eraseFromParent();
      continue;
    }
  }

  Changed = true;
  return Changed;

}

PreservedAnalyses ControlFlowFlattening::run(Module &M,
                                             ModuleAnalysisManager &MAM) {
  std::unique_ptr<RandomNumberGenerator> RNG = M.createRNG(name());

  bool Changed = false;
  for (Function& F : M) {
    bool fChanged = runOnFunction(F, *RNG);

    if (fChanged) {
      reg2mem(F);
    }

    Changed |= fChanged;
  }

  SINFO("[{}] Done!", name());
  return Changed ? PreservedAnalyses::none() :
                   PreservedAnalyses::all();

}
}


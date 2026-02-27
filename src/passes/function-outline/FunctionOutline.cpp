//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/function-outline/FunctionOutline.hpp"
#include "omvll/passes/function-outline/FunctionOutlineOpt.hpp"
#include "omvll/utils.hpp"

using namespace llvm;
using namespace omvll;

namespace omvll {

static bool isStackFrameDependentIntrinsic(Intrinsic::ID ID) {
  switch (ID) {
  case Intrinsic::vastart:
  case Intrinsic::vacopy:
  case Intrinsic::vaend:
  case Intrinsic::localaddress:
  case Intrinsic::localescape:
  case Intrinsic::localrecover:
  case Intrinsic::stacksave:
  case Intrinsic::stackrestore:
  case Intrinsic::get_dynamic_area_offset:
  case Intrinsic::stackprotector:
    return true;
  default:
    return false;
  }
}

static bool outliningMayBeUnfavorable(const BasicBlock &BB) {
  for (const Instruction &I : BB) {
    if (auto *II = dyn_cast<IntrinsicInst>(&I))
      if (isStackFrameDependentIntrinsic(II->getIntrinsicID()))
        return true;

    if (auto *CB = dyn_cast<CallBase>(&I)) {
      if (CB->isMustTailCall())
        return true;
      if (CB->isInlineAsm())
        return true;
      if (isa<CallBrInst>(CB))
        return true;
    }
  }
  return false;
}

static bool isOutlineCandidate(const BasicBlock &BB) {
  if (BB.size() < 3)
    return false;

  if (BB.hasAddressTaken() || isEHBlock(BB) || containsSwiftErrorAlloca(BB) ||
      outliningMayBeUnfavorable(BB))
    return false;

  const Instruction *T = BB.getTerminator();
  if (isa<InvokeInst>(T) || isa<ResumeInst>(T) || isa<CallBrInst>(T))
    return false;
  return true;
}

static bool
hasSwiftErrorOrSwiftSelfAttribute(const SetVector<Value *> &Inputs) {
  for (Value *V : Inputs) {
    if (auto *AI = dyn_cast<AllocaInst>(V); AI && AI->isSwiftError())
      return true;
    if (auto *Arg = dyn_cast<Argument>(V); Arg && Arg->hasSwiftErrorAttr())
      return true;
    if (any_of(V->users(), [&](const auto *U) {
          if (auto *CB = dyn_cast<CallBase>(U)) {
            for (unsigned I = 0; I < CB->arg_size(); ++I)
              if (CB->getArgOperand(I) == V &&
                  (CB->paramHasAttr(I, Attribute::SwiftError) ||
                   CB->paramHasAttr(I, Attribute::SwiftSelf)))
                return true;
          }
          return false;
        }))
      return true;
  }
  return false;
}

static void eraseLifetimeMarkers(Function *F) {
  for (BasicBlock &BB : *F)
    for (Instruction &I : llvm::make_early_inc_range(BB))
      if (auto *II = dyn_cast<LifetimeIntrinsic>(&I))
        if (II->getIntrinsicID() == Intrinsic::lifetime_start ||
            II->getIntrinsicID() == Intrinsic::lifetime_end)
          II->eraseFromParent();
}

bool FunctionOutline::process(Function &F, LLVMContext &Ctx,
                              unsigned Probability) {
  SmallVector<BasicBlock *, 32> ToOutline;
  ToOutline.reserve(F.size());

  // Collect basic blocks to outline.
  auto ShouldOutline = [&]() { return (*RNG)() % 100U < Probability; };
  for (BasicBlock &BB : F) {
    if (&BB == &F.getEntryBlock())
      continue;
    // Some further checks in addition to the ones in
    // `CodeExtractor::isEligible`.
    if (!isOutlineCandidate(BB))
      continue;
    if (ShouldOutline())
      ToOutline.push_back(&BB);
  }

  if (ToOutline.empty())
    return false;

  unsigned Outlined = 0;
  for (BasicBlock *BB : ToOutline) {
    SmallVector<BasicBlock *, 1> Region{BB};
    CodeExtractor CE(Region);
    // Missed something?
    if (!CE.isEligible())
      continue;

    // For now also avoid inputs w/ swifterror or swiftself attribute.
    SetVector<Value *> Inputs, Outputs, SinkCands;
    CE.findInputsOutputs(Inputs, Outputs, SinkCands);
    if (hasSwiftErrorOrSwiftSelfAttribute(Inputs))
      continue;

    // Outline region and replace the original block with a call-site to the
    // newly-created function.
    CodeExtractorAnalysisCache CEAC(F);
    if (auto *OutlinedFn = CE.extractCodeRegion(CEAC)) {
      SDEBUG("[{}] New outlined function {}", name(), OutlinedFn->getName());
      eraseLifetimeMarkers(OutlinedFn);
      ++Outlined;
    }
  }

  if (!Outlined)
    return false;

  SDEBUG("[{}] Outlined {} basic blocks within function {}", name(),
         std::to_string(Outlined), F.getName());

  // Erase lifetime markers.
  eraseLifetimeMarkers(&F);
  return true;
}

PreservedAnalyses FunctionOutline::run(Module &M, ModuleAnalysisManager &MAM) {
  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  PyConfig &Config = PyConfig::instance();
  SINFO("[{}] Executing on module {}", name(), M.getName());

  std::vector<std::pair<Function *, unsigned>> ToVisit;
  for (Function &F : M) {
    FunctionOutlineOpt Opt = Config.getUserConfig()->functionOutline(&M, &F);

    auto *P = std::get_if<FunctionOutlineWithProbability>(&Opt);
    if (P && !isFunctionGloballyExcluded(&F) && !F.isDeclaration() &&
        !F.isIntrinsic() && !F.getName().starts_with("__omvll") &&
        !isCoroutine(&F))
      ToVisit.emplace_back(&F, P->Probability);
  }

  if (ToVisit.empty())
    return PreservedAnalyses::all();

  bool Changed = false;
  LLVMContext &Ctx = M.getContext();
  RNG = M.createRNG(name());

  for (const auto &[F, P] : ToVisit)
    Changed |= process(*F, Ctx, P);

  if (Changed) {
    SmallVector<Function *, 4> ToRemove;
    for (Function &F : M) {
      if (F.isIntrinsic()) {
        auto ID = F.getIntrinsicID();
        if (ID == Intrinsic::lifetime_start || ID == Intrinsic::lifetime_end) {
          if (F.use_empty())
            ToRemove.push_back(&F);
        }
      }
    }
    for (auto *F : ToRemove)
      F->eraseFromParent();
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll

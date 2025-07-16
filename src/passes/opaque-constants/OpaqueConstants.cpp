//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/Metadata.hpp"
#include "omvll/passes/opaque-constants/OpaqueConstants.hpp"
#include "omvll/utils.hpp"
#include "omvll/visitvariant.hpp"

#include "GenOpaque.hpp"

using namespace llvm;

namespace omvll {

inline bool isEligible(const Instruction &I) {
  return isa<LoadInst>(I) || isa<StoreInst>(I) || isa<BinaryOperator>(I);
}

inline bool isSkip(const OpaqueConstantsOpt &Opt) {
  return std::get_if<OpaqueConstantsSkip>(&Opt) != nullptr;
}

bool OpaqueConstants::process(Instruction &I, Use &Op, ConstantInt &CI,
                              OpaqueConstantsOpt *Opt) {
  if (!isEligible(I))
    return false;

  // Special processing for 0 values.
  if (CI.isZero()) {
    if (Opt) {
      bool ShouldProtect =
          std::visit(overloaded{
                         [](OpaqueConstantsSkip &) { return false; },
                         [](OpaqueConstantsBool &V) { return V.Value; },
                         [](OpaqueConstantsLowerLimit &V) { return false; },
                         [](OpaqueConstantsSet &V) { return V.contains(0); },
                     },
                     *Opt);

      if (!ShouldProtect)
        return false;
    }

    Value *NewZero = getOpaqueZero(I, CI.getType());
    if (!NewZero) {
      SWARN("[{}] Cannot opaque {}", name(), ToString(CI));
      return false;
    }
    Op.set(NewZero);
    return true;
  }

  // Special processing for 1 values.
  if (CI.isOne()) {
    if (Opt) {
      bool ShouldProtect = std::visit(
          overloaded{
              [](OpaqueConstantsSkip &) { return false; },
              [](OpaqueConstantsBool &V) { return V.Value; },
              [](OpaqueConstantsLowerLimit &V) { return V.Value >= 1; },
              [](OpaqueConstantsSet &V) { return V.contains(1); },
          },
          *Opt);

      if (!ShouldProtect)
        return false;
    }

    Value *NewOne = getOpaqueOne(I, CI.getType());
    if (!NewOne) {
      SWARN("[{}] Cannot opaque {}", name(), ToString(CI));
      return false;
    }
    Op.set(NewOne);
    return true;
  }

  bool ShouldProtect =
      std::visit(overloaded{
                     [](OpaqueConstantsSkip &) { return false; },
                     [](OpaqueConstantsBool &V) { return V.Value; },
                     [&CI](OpaqueConstantsLowerLimit &V) {
                       return CI.getLimitedValue() > V.Value;
                     },
                     [&CI](OpaqueConstantsSet &V) {
                       static constexpr uint64_t Magic = 0x4208D8DF2C6415BC;
                       const uint64_t LV = CI.getLimitedValue(Magic);
                       if (LV == Magic)
                         return true;
                       return !V.empty() && V.contains(LV);
                     },
                 },
                 *Opt);

  if (!ShouldProtect)
    return false;

  Value *NewCst = getOpaqueCst(I, CI);
  if (!NewCst) {
    SWARN("[{}] Cannot opaque {}", name(), ToString(CI));
    return false;
  }
  Op.set(NewCst);
  return true;
}

Value *OpaqueConstants::getOpaqueZero(Instruction &I, Type *Ty) {
  static constexpr auto MaxCases = 3;
  static_assert(RandomNumberGenerator::max() >= MaxCases);
  std::uniform_int_distribution<uint8_t> Dist(1, MaxCases);

  uint8_t Sel = Dist(*RNG);
  switch (Sel) {
  case 1:
    return getOpaqueZero1(I, Ty, *RNG);
  case 2:
    return getOpaqueZero2(I, Ty, *RNG);
  case 3:
    return getOpaqueZero3(I, Ty, *RNG);
  default: {
    SWARN("[{}] RNG number ({}) out of range for generating opaque zero",
          name(), Sel);
    return nullptr;
  }
  }
}

Value *OpaqueConstants::getOpaqueOne(Instruction &I, Type *Ty) {
  static constexpr auto MaxCases = 3;
  static_assert(RandomNumberGenerator::max() >= MaxCases);
  std::uniform_int_distribution<uint8_t> Dist(1, MaxCases);

  uint8_t Sel = Dist(*RNG);
  switch (Sel) {
  case 1:
    return getOpaqueOne1(I, Ty, *RNG);
  case 2:
    return getOpaqueOne2(I, Ty, *RNG);
  case 3:
    return getOpaqueOne3(I, Ty, *RNG);
  default: {
    SWARN("[{}] RNG number ({}) out of range for generating opaque one", name(),
          Sel);
    return nullptr;
  }
  }
}

Value *OpaqueConstants::getOpaqueCst(Instruction &I, const ConstantInt &CI) {
  static constexpr auto MaxCases = 3;
  static_assert(RandomNumberGenerator::max() >= MaxCases);
  std::uniform_int_distribution<uint8_t> Dist(1, MaxCases);

  uint8_t Sel = Dist(*RNG);
  switch (Sel) {
  case 1:
    return getOpaqueConst1(I, CI, *RNG);
  case 2:
    return getOpaqueConst2(I, CI, *RNG);
  case 3:
    return getOpaqueConst3(I, CI, *RNG);
  default: {
    SWARN("[{}] RNG number ({}) out of range for generating opaque value",
          name(), Sel);
    return nullptr;
  }
  }
}

bool OpaqueConstants::process(Instruction &I, OpaqueConstantsOpt *Opt) {
  bool Changed = false;

#ifdef OMVLL_DEBUG
  std::string InstStr = ToString(I);
#endif
  for (Use &Op : I.operands())
    if (auto *CI = dyn_cast<ConstantInt>(Op))
      Changed |= process(I, Op, *CI, Opt);

#ifdef OMVLL_DEBUG
  if (Changed)
    SDEBUG("[{}] Opaquize constant in instruction {}", name(), InstStr);
#endif

  return Changed;
}

bool OpaqueConstants::runOnBasicBlock(llvm::BasicBlock &BB,
                                      OpaqueConstantsOpt *Opt) {
  bool Changed = false;

  for (Instruction &I : BB) {
    if (hasObf(I, MetaObfTy::OpaqueCst)) {
      OpaqueConstantsOpt Force = OpaqueConstantsBool(true);
      Changed |= process(I, &Force);
    } else if (Opt) {
      Changed |= process(I, Opt);
    }
  }

  return Changed;
}

PreservedAnalyses OpaqueConstants::run(Module &M, ModuleAnalysisManager &FAM) {
  bool Changed = false;
  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  PyConfig &Config = PyConfig::instance();
  SINFO("[{}] Executing on module {}", name(), M.getName());
  RNG = M.createRNG(name());

  for (Function &F : M) {
    if (isFunctionGloballyExcluded(&F))
      continue;

    OpaqueConstantsOpt Opt = Config.getUserConfig()->obfuscateConstants(&M, &F);
    OpaqueConstantsOpt *Inserted = nullptr;
    if (isSkip(Opt))
      continue;

    auto Ret = Opts.insert({&F, std::move(Opt)});
    if (Ret.second)
      Inserted = &Ret.first->second;

    for (BasicBlock &BB : F) {
      // Don't try opaque constants when potentially handling infinite loops.
      if (is_contained(successors(&BB), &BB))
        continue;
      Changed |= runOnBasicBlock(BB, Inserted);
    }
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll

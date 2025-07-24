//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <utility>

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
  return isa<LoadInst>(I) || isa<StoreInst>(I) || isa<BinaryOperator>(I) ||
         isa<ReturnInst>(I);
}

inline bool isSkip(const OpaqueConstantsOpt &Opt) {
  return std::get_if<OpaqueConstantsSkip>(&Opt) != nullptr;
}

bool OpaqueConstants::process(Instruction &I, Use &Op, ConstantInt &CI,
                              OpaqueConstantsOpt *Opt) {
  if (!isEligible(I))
    return false;

  Function &F = *I.getFunction();
  OpaqueContext *Ctx = getOrCreateContext(F);
  if (!Ctx) {
    SWARN("[{}] Cannot opaque {}", name(), ToString(F));
    return false;
  }

  auto ShouldProtect = [&](const ConstantInt &CI) -> bool {
    return std::visit(overloaded{
                          [](OpaqueConstantsSkip &) { return false; },
                          [](OpaqueConstantsBool &V) { return V.Value; },
                          [&](OpaqueConstantsLowerLimit &V) {
                            if (CI.isZero())
                              return false;
                            if (CI.isOne())
                              return V.Value >= 1;
                            return CI.getLimitedValue() > V.Value;
                          },
                          [&](OpaqueConstantsSet &V) {
                            if (CI.isZero())
                              return V.contains(0);
                            if (CI.isOne())
                              return V.contains(1);
                            static constexpr uint64_t Magic =
                                0x4208D8DF2C6415BC;
                            const uint64_t LV = CI.getLimitedValue(Magic);
                            if (LV == Magic)
                              return true;
                            return !V.empty() && V.contains(LV);
                          },
                      },
                      *Opt);
  };

  if (Opt && !ShouldProtect(CI))
    return false;

  Value *NewVal = nullptr;

  if (CI.isZero()) {
    // Special processing for 0 values.
    NewVal = getOpaqueZero(I, *Ctx, CI.getType());
  } else if (CI.isOne()) {
    // Special processing for 1 values.
    NewVal = getOpaqueOne(I, *Ctx, CI.getType());
  } else {
    NewVal = getOpaqueCst(I, *Ctx, CI);
  }

  if (!NewVal) {
    SWARN("[{}] Cannot opaque {}", name(), ToString(CI));
    return false;
  }

  Op.set(NewVal);
  return true;
}

template <typename CasesT, typename... ArgsT>
static Value *
getOpaqueRandomRoutine(std::unique_ptr<llvm::RandomNumberGenerator> &RNG,
                       const CasesT &Cases, ArgsT &&...Args) {
  static constexpr auto MaxCases = 3;
  static_assert(RandomNumberGenerator::max() >= MaxCases);
  std::uniform_int_distribution<size_t> Dist(0, MaxCases - 1);
  const auto Idx = Dist(*RNG);
  assert(Idx < Cases.size() && "Shouldn't have Idx out of bounds?");

  if (Value *V = (*Cases[Idx])(std::forward<ArgsT>(Args)..., *RNG))
    return V;
  return nullptr;
}

Value *OpaqueConstants::getOpaqueZero(Instruction &I, OpaqueContext &Ctx,
                                      Type *Ty) {
  static constexpr std::array Cases{&getOpaqueZero1, &getOpaqueZero2,
                                    &getOpaqueZero3};
  return getOpaqueRandomRoutine(RNG, Cases, I, Ctx, Ty);
}

Value *OpaqueConstants::getOpaqueOne(Instruction &I, OpaqueContext &Ctx,
                                     Type *Ty) {
  static constexpr std::array Cases{&getOpaqueOne1, &getOpaqueOne2,
                                    &getOpaqueOne3};
  return getOpaqueRandomRoutine(RNG, Cases, I, Ctx, Ty);
}

Value *OpaqueConstants::getOpaqueCst(Instruction &I, OpaqueContext &Ctx,
                                     const ConstantInt &CI) {
  static constexpr std::array Cases{&getOpaqueConst1, &getOpaqueConst2,
                                    &getOpaqueConst3};
  return getOpaqueRandomRoutine(RNG, Cases, I, Ctx, CI);
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

OpaqueContext *OpaqueConstants::getOrCreateContext(Function &F) {
  if (auto It = OpaqueCtx.find(&F); It != OpaqueCtx.end())
    return &It->second;

  const DataLayout &DL = F.getParent()->getDataLayout();
  Align PtrAlign(DL.getPointerSize());
  OpaqueContext &Ctx = OpaqueCtx[&F];

  IRBuilder<NoFolder> IRB(&*F.getEntryBlock().getFirstInsertionPt());
  Ctx.T1 = IRB.CreateAlloca(IRB.getInt64Ty(), nullptr, "opaque.t1");
  Ctx.T2 = IRB.CreateAlloca(IRB.getInt64Ty(), nullptr, "opaque.t2");

  IRB.CreateAlignedStore(IRB.CreatePtrToInt(Ctx.T2, IRB.getInt64Ty()), Ctx.T1,
                         PtrAlign);
  IRB.CreateAlignedStore(IRB.CreatePtrToInt(Ctx.T1, IRB.getInt64Ty()), Ctx.T2,
                         PtrAlign);

  return &Ctx;
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
    if (isFunctionGloballyExcluded(&F) || F.isDeclaration() ||
        F.isIntrinsic() || F.getName().starts_with("__omvll"))
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

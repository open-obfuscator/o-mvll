//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/RandomNumberGenerator.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/indirect-call/IndirectCall.hpp"
#include "omvll/passes/indirect-call/IndirectCallOpt.hpp"
#include "omvll/utils.hpp"

using namespace llvm;
using namespace omvll;

namespace omvll {

static uint32_t
getRandomShareAligned(std::unique_ptr<RandomNumberGenerator> &RNG) {
  size_t Max = std::numeric_limits<uint32_t>::max();
  std::uniform_int_distribution<uint32_t> Dist(1, Max);
  return Dist(*RNG) & 0xFFFFFF00ULL;
}

/// This pass rewrites direct calls into an indirect call through an address
/// reconstructed given two random shares:
///
///   Address = Share2 – Share1
///
/// Where Share1 is a randomly chosen aligned value, and Share2 is the sum of
/// Share1 and the actual callee address.
bool IndirectCall::process(Function &F, const DataLayout &DL,
                           LLVMContext &Ctx) {
  Module &M = *F.getParent();
  auto *IntPtrTy = DL.getIntPtrType(Ctx);
  SmallVector<Constant *, 32> Shares1, Shares2;
  SmallVector<CallInst *, 32> DirectCalls;

  // Gather direct function calls, candidates to be converted to indirect ones.
  for (Instruction &I : instructions(F)) {
    auto *CI = dyn_cast<CallInst>(&I);
    if (!CI)
      continue;

    Function *Callee = CI->getCalledFunction();
    if (!Callee || Callee->isIntrinsic() ||
        Callee->hasFnAttribute(Attribute::AlwaysInline))
      continue;

    Constant *TargetAddr = ConstantExpr::getPtrToInt(Callee, IntPtrTy);
    APInt RandomVal(IntPtrTy->getBitWidth(), getRandomShareAligned(RNG));
    Constant *Share1 = ConstantInt::get(IntPtrTy, RandomVal);
    Constant *Share2 = ConstantExpr::getAdd(TargetAddr, Share1);

    Shares1.push_back(Share1);
    Shares2.push_back(Share2);
    DirectCalls.push_back(CI);
  }

  if (DirectCalls.empty())
    return false;

  // Materialize all the shares into two constant global arrays.
  ArrayType *ArrTy = ArrayType::get(IntPtrTy, Shares1.size());
  auto *GVAddrShares1 =
      new GlobalVariable(M, ArrTy, true, GlobalValue::InternalLinkage,
                         ConstantArray::get(ArrTy, Shares1), ".icall.shares1");
  auto *GVAddrShares2 =
      new GlobalVariable(M, ArrTy, true, GlobalValue::InternalLinkage,
                         ConstantArray::get(ArrTy, Shares2), ".icall.shares2");

  IRBuilder<> Builder(Ctx);
  auto *Zero = ConstantInt::get(Type::getInt64Ty(Ctx), 0);

  // Rewrite each call-site by loading the shares and computing the final
  // target address as the difference between Share2 and Share1.
  for (auto [Idx, CI] : enumerate(DirectCalls)) {
    Builder.SetInsertPoint(CI);
    Constant *Index = ConstantInt::get(Type::getInt64Ty(Ctx), Idx);

    Value *PtrS1 =
        Builder.CreateInBoundsGEP(ArrTy, GVAddrShares1, {Zero, Index});
    Value *PtrS2 =
        Builder.CreateInBoundsGEP(ArrTy, GVAddrShares2, {Zero, Index});

    Value *Share1 = Builder.CreateLoad(IntPtrTy, PtrS1);
    Value *Share2 = Builder.CreateLoad(IntPtrTy, PtrS2);

    Value *Address = Builder.CreateSub(Share2, Share1);
    SDEBUG("[{}] Rewriting call-site: {}", name(), ToString(*CI));

    // Reconstruct the callee address.
    CI->setCalledOperand(Builder.CreateIntToPtr(Address, Builder.getPtrTy(0)));
  }

  return true;
}

PreservedAnalyses IndirectCall::run(Module &M, ModuleAnalysisManager &MAM) {
  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  PyConfig &Config = PyConfig::instance();
  LLVMContext &Ctx = M.getContext();
  const auto &DL = M.getDataLayout();
  RNG = M.createRNG(name());
  SINFO("[{}] Executing on module {}", name(), M.getName());

  for (Function &F : M) {
    IndirectCallOpt Opt = Config.getUserConfig()->indirectCall(&M, &F);
    if (!Opt || !*Opt)
      continue;

    if (isFunctionGloballyExcluded(&F) || F.isDeclaration() ||
        F.isIntrinsic() || F.getName().starts_with("__omvll"))
      continue;

    Changed |= process(F, DL, Ctx);
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll

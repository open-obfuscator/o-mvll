//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/Support/RandomNumberGenerator.h"

#include "omvll/passes/Metadata.hpp"
#include "omvll/passes/opaque-constants/OpaqueConstants.hpp"
#include "omvll/utils.hpp"

#include "GenOpaque.hpp"

using namespace llvm;

namespace omvll {

/* ========= Zero Value Gen ========= */
Value *getOpaqueZero1(Instruction &I, Type *Ty,
                      RandomNumberGenerator &RNG) {
  IRBuilder<NoFolder> IRB(&I);

  // Create a dummy stack allocation and use it as a runtime seed.
  AllocaInst *Alloca = IRB.CreateAlloca(Ty);
  Value *Seed = IRB.CreateLoad(Ty, Alloca);

  std::uniform_int_distribution<uint64_t> Dist(1, 0xFFFFFFFFFFFF);
  uint64_t RandVal = Dist(RNG);

  Value *Masked = IRB.CreateXor(ConstantInt::get(Ty, RandVal), Seed);
  addMetadata(*cast<Instruction>(Masked), MetaObf(MetaObfTy::OpaqueOp, 3LLU));
  Value *X = IRB.CreateXor(Masked, Seed);
  addMetadata(*cast<Instruction>(X), MetaObf(MetaObfTy::OpaqueOp, 3LLU));
  Value *Y = IRB.CreateXor(Masked, Seed);
  addMetadata(*cast<Instruction>(Y), MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  // (X ^ Y) - (Y ^ X) == 0
  Value *Xor1 = IRB.CreateXor(X, Y);
  addMetadata(*cast<Instruction>(Xor1), MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  Value *Xor2 = IRB.CreateXor(Y, X);
  addMetadata(*cast<Instruction>(Xor2), MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  Value *Zero = IRB.CreateSub(Xor1, Xor2);
  addMetadata(*cast<Instruction>(Zero), MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  return Zero; // Equals zero
}

Value *getOpaqueZero2(Instruction &I, Type *Ty,
                      RandomNumberGenerator &RNG) {
  return getOpaqueZero1(I, Ty, RNG);
}

Value *getOpaqueZero3(Instruction &I, Type *Ty,
                      RandomNumberGenerator &RNG) {
  return getOpaqueZero1(I, Ty, RNG);
}

/* ========= One Value Gen ========= */
Value *getOpaqueOne1(Instruction &I, Type *Ty, RandomNumberGenerator &RNG) {
  IRBuilder<NoFolder> IRB(&I);

  AllocaInst *Alloca = IRB.CreateAlloca(Ty);
  Value *Seed = IRB.CreateLoad(Ty, Alloca);

  std::uniform_int_distribution<uint64_t> Dist(3, 101);
  uint64_t N = Dist(RNG);
  if (N % 2 == 0) N++; // Ensure odd

  Value *Masked = IRB.CreateXor(ConstantInt::get(Ty, N), Seed);
  addMetadata(*cast<Instruction>(Masked), MetaObf(MetaObfTy::OpaqueOp, 3LLU));
  Value *Even = IRB.CreateXor(Masked, Seed); // Reconstruct N (odd)
  addMetadata(*cast<Instruction>(Even), MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  Value *Or = IRB.CreateOr(Even, ConstantInt::get(Ty, 1)); // Will be odd
  addMetadata(*cast<Instruction>(Or), MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  Value *Opaque = IRB.CreateAnd(Or, ConstantInt::get(Ty, 1)); // Always 1
  addMetadata(*cast<Instruction>(Opaque), MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  return Opaque;
}


Value *getOpaqueOne2(Instruction &I, Type *Ty,
                     RandomNumberGenerator &RNG) {
  return getOpaqueOne1(I, Ty, RNG);
}

Value *getOpaqueOne3(Instruction &I, Type *Ty,
                     RandomNumberGenerator &RNG) {
  return getOpaqueOne1(I, Ty, RNG);
}

/* ========= Value != {0, 1} Gen ========= */
Value *getOpaqueConst1(Instruction &I, const ConstantInt &CI,
                       RandomNumberGenerator &RNG) {
  uint64_t Val = CI.getLimitedValue();

  if (Val <= 1 || Val == std::numeric_limits<uint64_t>::max())
    return nullptr;

  IRBuilder<NoFolder> IRB(&I);
  Type *Ty = CI.getType();

  // Split constant into two random parts: LHS + RHS = Val.
  std::uniform_int_distribution<uint64_t> Dist(1, Val - 1);
  uint64_t Split = Dist(RNG);

  uint64_t LHS = Val - Split;
  uint64_t RHS = Split;

  Value *ConstLHS = ConstantInt::get(Ty, LHS);
  Value *ConstRHS = ConstantInt::get(Ty, RHS);

  // Introduce basic variability via a dummy condition (opaque bool).
  Value *OpaqueCond = IRB.CreateICmpEQ(ConstLHS, ConstLHS);
  addMetadata(*cast<Instruction>(OpaqueCond), MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  // Use select to choose between two forms of the same value.
  Value *MaskedLHS = IRB.CreateXor(ConstLHS, ConstantInt::get(Ty, 0));
  addMetadata(*cast<Instruction>(MaskedLHS), MetaObf(MetaObfTy::OpaqueOp, 3LLU));
  Value *MaskedRHS = IRB.CreateXor(ConstRHS, ConstantInt::get(Ty, 0));
  addMetadata(*cast<Instruction>(MaskedRHS), MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  Value *AltLHS = IRB.CreateSelect(OpaqueCond, MaskedLHS, ConstLHS);
  addMetadata(*cast<Instruction>(AltLHS), MetaObf(MetaObfTy::OpaqueOp, 3LLU));
  Value *AltRHS = IRB.CreateSelect(OpaqueCond, MaskedRHS, ConstRHS);
  addMetadata(*cast<Instruction>(AltRHS), MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  // Final add.
  Value *Sum = IRB.CreateAdd(AltLHS, AltRHS);

  if (auto *Inst = dyn_cast<Instruction>(Sum))
    addMetadata(*Inst, MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  return Sum;
}

Value *getOpaqueConst2(Instruction &I, const ConstantInt &CI,
                       RandomNumberGenerator &RNG) {
  return getOpaqueConst1(I, CI, RNG);
}

Value *getOpaqueConst3(Instruction &I, const ConstantInt &CI,
                       RandomNumberGenerator &RNG) {
  return getOpaqueConst1(I, CI, RNG);
}

} // end namespace omvll

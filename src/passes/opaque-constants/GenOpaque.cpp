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

  // Generate a random 64-bit constant (limited to 48 bits if safer for embedded platforms)
  std::uniform_int_distribution<uint64_t> Dist(1, 0xFFFFFFFFFFFF);
  uint64_t RandVal = Dist(RNG);

  // Choose two identical constants
  auto *Cst = ConstantInt::get(Ty, RandVal);
  Value *X = Cst;
  Value *Y = Cst;

  // (X ^ Y) - (Y ^ X) == 0
  Value *Xor1 = IRB.CreateXor(X, Y);
  if (auto *Inst = dyn_cast<Instruction>(Xor1))
    addMetadata(*Inst, MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  Value *Xor2 = IRB.CreateXor(Y, X);
  if (auto *Inst = dyn_cast<Instruction>(Xor2))
    addMetadata(*Inst, MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  Value *MBA = IRB.CreateSub(Xor1, Xor2);
  if (auto *Inst = dyn_cast<Instruction>(MBA))
    addMetadata(*Inst, MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  Value *Zero = IRB.CreateIntCast(MBA, Ty, false);
  if (auto *Inst = dyn_cast<Instruction>(Zero))
    addMetadata(*Inst, MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  return Zero;
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

  std::uniform_int_distribution<uint64_t> Dist(3, 101);
  uint64_t N = Dist(RNG);
  if (N % 2 == 0) N++;  // ensure odd

  Value *Even = ConstantInt::get(Ty, N);
  Value *One  = ConstantInt::get(Ty, 1);
  Value *Or   = IRB.CreateOr(Even, One);
  if (auto *Inst = dyn_cast<Instruction>(Or))
    addMetadata(*Inst, MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  Value *Opaque = IRB.CreateAnd(Or, One); // (X | 1) & 1 => 1
  if (auto *Inst = dyn_cast<Instruction>(Opaque))
    addMetadata(*Inst, MetaObf(MetaObfTy::OpaqueOp, 3LLU));

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

  std::uniform_int_distribution<uint64_t> Dist(1, Val - 1);
  uint64_t Split = Dist(RNG);

  uint64_t LHS = Val - Split;
  uint64_t RHS = Split;

  IRBuilder<NoFolder> IRB(&I);
  auto *Ty = CI.getType();

  Value *OpaqueLHS = ConstantInt::get(Ty, LHS);
  Value *OpaqueRHS = ConstantInt::get(Ty, RHS);

  Value *Sum = IRB.CreateAdd(OpaqueLHS, OpaqueRHS);

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

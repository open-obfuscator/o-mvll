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

static Value *attachMDOpaqueCst(Value *I) {
  addMetadata(*cast<Instruction>(I), MetaObf(MetaObfTy::OpaqueCst, 3ULL));
  return I;
}

/* ========= Zero Value Gen ========= */
Value *getOpaqueZero1(Instruction &I, OpaqueContext &Ctx, Type *Ty,
                      RandomNumberGenerator &RNG) {
  IRBuilder<NoFolder> IRB(&I);

  Value *Seed1 = IRB.CreateLoad(Ty, Ctx.T1, true);
  Value *Seed2 = IRB.CreateLoad(Ty, Ctx.T2, true);

  Value *Masked = attachMDOpaqueCst(IRB.CreateXor(Seed1, Seed2));
  Value *X = attachMDOpaqueCst(IRB.CreateXor(Masked, Seed1));
  Value *Y = attachMDOpaqueCst(IRB.CreateXor(Masked, Seed2));

  // (X ^ Y) - (Y ^ X) == 0
  Value *Xor1 = attachMDOpaqueCst(IRB.CreateXor(X, Y));
  Value *Xor2 = attachMDOpaqueCst(IRB.CreateXor(Y, X));
  Value *Zero = attachMDOpaqueCst(IRB.CreateSub(Xor1, Xor2));

  return Zero; // Equals zero
}

Value *getOpaqueZero2(Instruction &I, OpaqueContext &Ctx, Type *Ty,
                      RandomNumberGenerator &RNG) {
  return getOpaqueZero1(I, Ctx, Ty, RNG);
}

Value *getOpaqueZero3(Instruction &I, OpaqueContext &Ctx, Type *Ty,
                      RandomNumberGenerator &RNG) {
  return getOpaqueZero1(I, Ctx, Ty, RNG);
}

/* ========= One Value Gen ========= */
Value *getOpaqueOne1(Instruction &I, OpaqueContext &Ctx, Type *Ty,
                     RandomNumberGenerator &RNG) {
  IRBuilder<NoFolder> IRB(&I);

  std::uniform_int_distribution<uint64_t> Dist(3, 101);
  uint64_t N = Dist(RNG);
  if (N % 2 == 0)
    N++; // Ensure odd

  Value *Seed = IRB.CreateLoad(Ty, Ctx.T2, true);
  Value *Masked =
      attachMDOpaqueCst(IRB.CreateXor(ConstantInt::get(Ty, N), Seed));
  Value *Even = attachMDOpaqueCst(IRB.CreateXor(Masked, Seed));

  Value *Or = attachMDOpaqueCst(IRB.CreateOr(Even, ConstantInt::get(Ty, 1)));
  Value *OpaqueOne =
      attachMDOpaqueCst(IRB.CreateAnd(Or, ConstantInt::get(Ty, 1)));

  return OpaqueOne;
}

Value *getOpaqueOne2(Instruction &I, OpaqueContext &Ctx, Type *Ty,
                     RandomNumberGenerator &RNG) {
  return getOpaqueOne1(I, Ctx, Ty, RNG);
}

Value *getOpaqueOne3(Instruction &I, OpaqueContext &Ctx, Type *Ty,
                     RandomNumberGenerator &RNG) {
  return getOpaqueOne1(I, Ctx, Ty, RNG);
}

/* ========= Value != {0, 1} Gen ========= */
Value *getOpaqueConst1(Instruction &I, OpaqueContext &Ctx,
                       const ConstantInt &CI, RandomNumberGenerator &RNG) {
  uint64_t Val = CI.getLimitedValue();
  if (Val <= 1 || Val == std::numeric_limits<uint64_t>::max())
    return nullptr;

  std::uniform_int_distribution<uint64_t> Dist(1, Val - 1);
  uint64_t RHS = Dist(RNG);
  uint64_t LHS = Val - RHS;

  Type *Ty = CI.getType();
  IRBuilder<NoFolder> IRB(&I);

  Value *V1 = IRB.CreateLoad(Ty, Ctx.T1, true);
  Value *V2 = IRB.CreateLoad(Ty, Ctx.T2, true);

  Value *Xor1 = attachMDOpaqueCst(IRB.CreateXor(V1, V2));
  Value *Xor2 = attachMDOpaqueCst(IRB.CreateXor(V2, V1));
  Value *OpaqueZero = attachMDOpaqueCst(IRB.CreateSub(Xor1, Xor2));

  Value *OpaqueLHS = attachMDOpaqueCst(
      IRB.CreateAdd(OpaqueZero, ConstantInt::get(Ty, LHS, false)));
  Value *OpaqueRHS = attachMDOpaqueCst(
      IRB.CreateAdd(OpaqueZero, ConstantInt::get(Ty, RHS, false)));

  IRB.CreateStore(OpaqueLHS, Ctx.T2, true);
  IRB.CreateStore(OpaqueRHS, Ctx.T1, true);

  Value *Add = attachMDOpaqueCst(IRB.CreateAdd(
      IRB.CreateLoad(Ty, Ctx.T2, true), IRB.CreateLoad(Ty, Ctx.T1, true)));
  return Add;
}

Value *getOpaqueConst2(Instruction &I, OpaqueContext &Ctx,
                       const ConstantInt &CI, RandomNumberGenerator &RNG) {
  return getOpaqueConst1(I, Ctx, CI, RNG);
}

Value *getOpaqueConst3(Instruction &I, OpaqueContext &Ctx,
                       const ConstantInt &CI, RandomNumberGenerator &RNG) {
  return getOpaqueConst1(I, Ctx, CI, RNG);
}

} // end namespace omvll

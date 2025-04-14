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

static constexpr uint8_t StackAlignment = 0x04;

/* ========= Zero Value Gen ========= */
Value *getOpaqueZero1(Instruction &I, OpaqueContext &C, Type *Ty,
                      RandomNumberGenerator &RNG) {
  IRBuilder<NoFolder> IRB(&I);

  auto *T1Ptr = IRB.CreatePointerCast(C.T1, Ty->getPointerTo());
  auto *T2Ptr = IRB.CreatePointerCast(C.T2, Ty->getPointerTo());

  auto *Tmp1 = IRB.CreateLoad(Ty, T1Ptr, true);
  auto *Tmp2 = IRB.CreateLoad(Ty, T2Ptr, true);

  Value *MBAXor = IRB.CreateXor(Tmp2, Tmp1);
  if (auto *Inst = dyn_cast<Instruction>(MBAXor))
    addMetadata(*Inst, MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  // MBA(x ^ y) - (x ^ y)
  Value *NewZero = IRB.CreateIntCast(
      IRB.CreateSub(MBAXor, IRB.CreateXor(Tmp2, Tmp1)), Ty, false);
  return NewZero;
}

Value *getOpaqueZero2(Instruction &I, OpaqueContext &C, Type *Ty,
                      RandomNumberGenerator &RNG) {
  return getOpaqueZero1(I, C, Ty, RNG);
}

Value *getOpaqueZero3(Instruction &I, OpaqueContext &C, Type *Ty,
                      RandomNumberGenerator &RNG) {
  return getOpaqueZero1(I, C, Ty, RNG);
}

/* ========= One Value Gen ========= */
Value *getOpaqueOne1(Instruction &I, OpaqueContext &C, Type *Ty,
                     RandomNumberGenerator &RNG) {
  std::uniform_int_distribution<uint8_t> Dist(1, 50);
  uint8_t Odd = Dist(RNG);
  if (Odd % 2 == 0)
    Odd += 1;

  IRBuilder<NoFolder> IRB(&I);
  auto *T2Addr = IRB.CreatePtrToInt(C.T2, IRB.getInt64Ty());
  auto *OddAddr =
      IRB.CreateAdd(T2Addr, ConstantInt::get(IRB.getInt64Ty(), Odd, false));

  auto *LSB =
      IRB.CreateAnd(OddAddr, ConstantInt::get(IRB.getInt64Ty(), 1, false));
  auto *OpaqueOne = IRB.CreateIntCast(LSB, Ty, false);

  return OpaqueOne;
}

Value *getOpaqueOne2(Instruction &I, OpaqueContext &C, Type *Ty,
                     RandomNumberGenerator &RNG) {
  return getOpaqueOne1(I, C, Ty, RNG);
}

Value *getOpaqueOne3(Instruction &I, OpaqueContext &C, Type *Ty,
                     RandomNumberGenerator &RNG) {
  return getOpaqueOne1(I, C, Ty, RNG);
}

/* ========= Value != {0, 1} Gen ========= */
Value *getOpaqueConst1(Instruction &I, OpaqueContext &C, const ConstantInt &CI,
                       RandomNumberGenerator &RNG) {
  uint64_t Val = CI.getLimitedValue();
  if (Val <= 1)
    return nullptr;

  std::uniform_int_distribution<uint8_t> Dist(1, Val);
  uint8_t Split = Dist(RNG);
  Module *M = I.getModule();
  GlobalVariable *GV = M->getGlobalVariable(OpaqueGVName, true);
  if (!GV)
    fatalError("Cannot find __omvll_opaque_gv");

  uint64_t LHS = Val - Split;
  uint64_t RHS = Split;

  auto *Ty = CI.getType();
  IRBuilder<NoFolder> IRB(&I);

  auto *GVPtr = IRB.CreatePointerCast(GV, Ty->getPointerTo());
  auto *T1Ptr = IRB.CreatePointerCast(C.T1, Ty->getPointerTo());

  auto *T2Addr = IRB.CreatePtrToInt(C.T2, IRB.getInt64Ty());
  auto *LSB = IRB.CreateAnd(
      T2Addr, ConstantInt::get(IRB.getInt64Ty(), StackAlignment, false));
  auto *OpaqueZero = IRB.CreateIntCast(LSB, Ty, false);

  auto *OpaqueLHS = IRB.CreateAdd(OpaqueZero, ConstantInt::get(Ty, LHS, false));
  auto *OpaqueRHS = IRB.CreateAdd(OpaqueZero, ConstantInt::get(Ty, RHS, false));

  IRB.CreateStore(OpaqueLHS, GVPtr, /*volatile=*/true);
  IRB.CreateStore(OpaqueRHS, T1Ptr, /*volatile=*/true);

  auto *Add = IRB.CreateAdd(IRB.CreateLoad(Ty, GVPtr, true),
                            IRB.CreateLoad(Ty, T1Ptr, true));
  if (auto *InstAdd = dyn_cast<Instruction>(Add))
    addMetadata(*InstAdd, MetaObf(MetaObfTy::OpaqueOp, 3LLU));

  return Add;
}

Value *getOpaqueConst2(Instruction &I, OpaqueContext &C, const ConstantInt &CI,
                       RandomNumberGenerator &RNG) {
  return getOpaqueConst1(I, C, CI, RNG);
}

Value *getOpaqueConst3(Instruction &I, OpaqueContext &C, const ConstantInt &CI,
                       RandomNumberGenerator &RNG) {
  return getOpaqueConst1(I, C, CI, RNG);
}

} // end namespace omvll

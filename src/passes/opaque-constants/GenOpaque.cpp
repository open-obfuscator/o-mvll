#include "omvll/passes/opaque-constants/OpaqueConstants.hpp"
#include "omvll/passes/Metadata.hpp"
#include "omvll/utils.hpp"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/NoFolder.h"

#include "llvm/Support/RandomNumberGenerator.h"

using namespace llvm;

namespace omvll {

static constexpr uint8_t STACK_ALIGNEMENT = 0x04;

/* Zero Value Gen
 * ============================================
 */

Value* GetOpaqueZero_1(Instruction& I, OpaqueContext& C, Type* Ty, RandomNumberGenerator& RNG) {
  IRBuilder<NoFolder> IRB(&I);

  auto* T1Ptr = IRB.CreatePointerCast(C.T1, Ty->getPointerTo());
  auto* T2Ptr = IRB.CreatePointerCast(C.T2, Ty->getPointerTo());

  auto* tmp1 = IRB.CreateLoad(Ty, T1Ptr, true);
  auto* tmp2 = IRB.CreateLoad(Ty, T2Ptr, true);

  Value* MBAXor = IRB.CreateXor(tmp2, tmp1);
  if (auto* Inst = dyn_cast<Instruction>(MBAXor)) {
    addMetadata(*Inst, MetaObf(MObfTy::OPAQUE_OP, 3llu));
  }
  Value* NewZero =
    IRB.CreateIntCast(
      IRB.CreateSub(
        // MBA(x ^ y) - (x ^ y)
        MBAXor, IRB.CreateXor(tmp2, tmp1)),
      Ty, false);
  return NewZero;
}

Value* GetOpaqueZero_2(Instruction& I, OpaqueContext& C, Type* Ty, RandomNumberGenerator& RNG) {
  return GetOpaqueZero_1(I, C, Ty, RNG);
}

Value* GetOpaqueZero_3(Instruction& I, OpaqueContext& C, Type* Ty, RandomNumberGenerator& RNG) {
  return GetOpaqueZero_1(I, C, Ty, RNG);
}


/* One Value Gen
 * ============================================
 */
Value* GetOpaqueOne_1(Instruction& I, OpaqueContext& C, Type* Ty, RandomNumberGenerator& RNG) {
  std::uniform_int_distribution<uint8_t> Dist(1, 50);
  uint8_t Odd = Dist(RNG);
  if (Odd % 2 == 0) { Odd += 1; }

  IRBuilder<NoFolder> IRB(&I);
  auto* T2Addr = IRB.CreatePtrToInt(C.T2, IRB.getInt64Ty());
  auto* OddAddr = IRB.CreateAdd(T2Addr, ConstantInt::get(IRB.getInt64Ty(), Odd, false));

  auto* LSB       = IRB.CreateAnd(OddAddr, ConstantInt::get(IRB.getInt64Ty(), 1, false));
  auto* OpaqueOne = IRB.CreateIntCast(LSB, Ty, false);
  return OpaqueOne;
}

Value* GetOpaqueOne_2(Instruction& I, OpaqueContext& C, Type* Ty, RandomNumberGenerator& RNG) {
  return GetOpaqueOne_1(I, C, Ty, RNG);
}

Value* GetOpaqueOne_3(Instruction& I, OpaqueContext& C, Type* Ty, RandomNumberGenerator& RNG) {
  return GetOpaqueOne_1(I, C, Ty, RNG);
}


/* Value != {0, 1} Gen
 * ============================================
 */

Value* GetOpaqueConst_1(Instruction& I, OpaqueContext& C, const ConstantInt& CI, RandomNumberGenerator& RNG) {
  uint64_t Val = CI.getLimitedValue();

  if (Val <= 1) {
    return nullptr;
  }

  std::uniform_int_distribution<uint8_t> Dist(1, Val);
  uint8_t Split = Dist(RNG);
  Module* M = I.getModule();
  GlobalVariable* GV = M->getGlobalVariable("__omvll_opaque_gv", true);

  if (GV == nullptr) {
    fatalError("Can't find __omvll_opaque_gv");
  }

  uint64_t LHS = Val - Split;
  uint64_t RHS = Split;

  auto* Ty = CI.getType();

  IRBuilder<NoFolder> IRB(&I);

  auto* GVPtr = IRB.CreatePointerCast(GV, Ty->getPointerTo());
  auto* T1Ptr = IRB.CreatePointerCast(C.T1, Ty->getPointerTo());

  auto* T2Addr = IRB.CreatePtrToInt(C.T2, IRB.getInt64Ty());
  auto* LSB    = IRB.CreateAnd(T2Addr, ConstantInt::get(IRB.getInt64Ty(), STACK_ALIGNEMENT, false));
  auto* OpaqueZero = IRB.CreateIntCast(LSB, Ty, false);

  //auto*
  auto* OpaqueLHS = IRB.CreateAdd(OpaqueZero, ConstantInt::get(Ty, LHS, false));
  auto* OpaqueRHS = IRB.CreateAdd(OpaqueZero, ConstantInt::get(Ty, RHS, false));

  IRB.CreateStore(OpaqueLHS, GVPtr, /*volatile=*/true);
  IRB.CreateStore(OpaqueRHS, T1Ptr, /*volatile=*/true);

  auto* Add = IRB.CreateAdd(
      IRB.CreateLoad(Ty, GVPtr, true),
      IRB.CreateLoad(Ty, T1Ptr, true));
  if (auto* InstAdd = dyn_cast<Instruction>(Add)) {
    addMetadata(*InstAdd, MetaObf(MObfTy::OPAQUE_OP, 3llu));
  }
  return Add;
}

Value* GetOpaqueConst_2(Instruction& I, OpaqueContext& C, const ConstantInt& CI, RandomNumberGenerator& RNG) {
  return GetOpaqueConst_1(I, C, CI, RNG);
}

Value* GetOpaqueConst_3(Instruction& I, OpaqueContext& C, const ConstantInt& CI, RandomNumberGenerator& RNG) {
  return GetOpaqueConst_1(I, C, CI, RNG);
}

}


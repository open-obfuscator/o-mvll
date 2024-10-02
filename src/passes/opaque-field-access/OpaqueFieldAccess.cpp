#include "omvll/passes/opaque-field-access/OpaqueFieldAccess.hpp"
#include "omvll/log.hpp"
#include "omvll/utils.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/ObfuscationConfig.hpp"
#include "omvll/passes/Metadata.hpp"

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Support/RandomNumberGenerator.h"


using namespace llvm;

namespace omvll {

bool OpaqueFieldAccess::runOnStructRead(BasicBlock& BB, LoadInst& Load,
                                        GetElementPtrInst& GEP, StructType& S)
{
  PyConfig& conf = PyConfig::instance();
  if (!conf.getUserConfig()->obfuscate_struct_access(BB.getModule(), BB.getParent(), &S)) {
    return false;
  }

  SINFO("[{}] Executing on module {} over structure {}", name(),
        BB.getModule()->getName(), S.getName());

  SmallVector<Value*, 2> indices(GEP.idx_begin(), GEP.idx_end());

  if (indices.size() != 2) {
    SWARN("[{}] Load {} expects 2 indices for the structure {}", name(),
          ToString(Load), S.getName());
    return false;
  }

  Value* Idx0 = indices[0];
  Value* Idx1 = indices[1];

  auto* ZeroVal = dyn_cast<ConstantInt>(Idx0);
  auto* OffVal  = dyn_cast<ConstantInt>(Idx1);

  if (ZeroVal == nullptr || OffVal == nullptr) {
    return false;
  }

  if (ZeroVal->getLimitedValue() != 0) {
    SWARN("[{}] Expecting a zero value for the getelementptr: {}", name(),
          ToString(GEP));
  }

  const DataLayout& DL = BB.getParent()->getParent()->getDataLayout();
  uint64_t ComputedOffset = DL.getStructLayout(&S)->getElementOffset(OffVal->getLimitedValue());

  SDEBUG("[{}] Obfuscating field READ access on {}->#{} (offset: {})", name(),
         S.getName(), OffVal->getLimitedValue(), ComputedOffset);

  IRBuilder<NoFolder> IRB(&Load);

  Value* opaqueOffset =
    IRB.CreateAdd(
      ConstantInt::get(IRB.getInt32Ty(), 0),
      ConstantInt::get(IRB.getInt32Ty(), ComputedOffset));

  if (auto* OpAdd = dyn_cast<Instruction>(opaqueOffset)) {
    addMetadata(*OpAdd, {MetaObf(OPAQUE_CST), MetaObf(OPAQUE_OP, 2llu)});
  }

  Value* newGEP =
    IRB.CreateBitCast(
      IRB.CreateInBoundsGEP(IRB.getInt8Ty(),
                            IRB.CreateBitCast(GEP.getPointerOperand(), IRB.getInt8PtrTy()),
                            opaqueOffset),
      GEP.getResultElementType()->getPointerTo());

  GEP.replaceAllUsesWith(newGEP);
  return true;
}


bool OpaqueFieldAccess::runOnBufferRead(BasicBlock& BB, LoadInst& Load,
                                        GetElementPtrInst& GEP, ConstantInt& CI)
{
  if (!hasObf(Load, MObfTy::PROTECT_FIELD_ACCESS)) {
    return false;
  }
  SDEBUG("[{}] Obfuscating buffer load access", name());
  IRBuilder<NoFolder> IRB(&Load);
  const uint64_t Val = CI.getLimitedValue();
  uint32_t lhs, rhs = 0;
  if (Val % 2 == 0) {
    lhs = Val / 2;
    rhs = Val / 2;
  } else {
    lhs = (Val - 1) / 2;
    rhs = (Val + 1) / 2;
  }

  Value* opaqueOffset =
    IRB.CreateAdd(
      ConstantInt::get(IRB.getInt32Ty(), lhs),
      ConstantInt::get(IRB.getInt32Ty(), rhs));

  if (auto* Op = dyn_cast<Instruction>(opaqueOffset)) {
    addMetadata(*Op, {MetaObf(OPAQUE_CST), MetaObf(OPAQUE_OP, 2llu)});
  }

  Value* newGEP =
    IRB.CreateBitCast(
      IRB.CreateInBoundsGEP(IRB.getInt8Ty(),
                            IRB.CreateBitCast(GEP.getPointerOperand(), IRB.getInt8PtrTy()),
                            opaqueOffset),
      GEP.getResultElementType()->getPointerTo());

  GEP.replaceAllUsesWith(newGEP);
  return true;
}

bool OpaqueFieldAccess::runOnConstantExprRead(BasicBlock& BB, LoadInst& Load, ConstantExpr& CE)
{
  if (CE.getNumOperands() < 3) {
    return false;
  }

  auto* GV       = dyn_cast<GlobalVariable>(CE.getOperand(0));
  auto* OpOffset = dyn_cast<ConstantInt>(CE.getOperand(2));
  if (GV == nullptr || OpOffset == nullptr) {
    return false;
  }
  Type *GVTy = GV->getValueType();

  if (!GVTy->isArrayTy()) {
    SDEBUG("[{}] Global variable {} won't be processed: non array type", name(),
           GV->getName());
    return false;
  }

  Type* ElTy = GVTy->getArrayElementType();

  PyConfig& conf = PyConfig::instance();
  if (!conf.getUserConfig()->obfuscate_variable_access(BB.getModule(), BB.getParent(), GV)) {
    return false;
  }

  SINFO("[{}] Executing on module {} over variable {}", name(),
        BB.getModule()->getName(), GV->getName());

  IRBuilder<NoFolder> IRB(&Load);
  Value* opaqueOffset =
    IRB.CreateSub(
      ConstantInt::get(IRB.getInt32Ty(), OpOffset->getLimitedValue()),
      ConstantInt::get(IRB.getInt32Ty(), 0));


  if (auto* Op = dyn_cast<Instruction>(opaqueOffset)) {
    addMetadata(*Op, {MetaObf(OPAQUE_CST), MetaObf(OPAQUE_OP, 2llu)});
  }


  Value* newGEP = IRB.CreateInBoundsGEP(ElTy,
                                        IRB.CreateBitCast(GV, ElTy->getPointerTo()),
                                        opaqueOffset);
  Load.setOperand(0, newGEP);
  return true;
}


bool OpaqueFieldAccess::runOnLoad(LoadInst& Load) {
  Value* LoadOp = Load.getPointerOperand();

  if (auto* GEP = dyn_cast<GetElementPtrInst>(LoadOp)) {
    Value* Target  = GEP->getPointerOperand();
    Type* TargetTy = Target->getType();

    if (TargetTy->isPointerTy() && !TargetTy->isOpaquePointerTy()) {
      TargetTy = TargetTy->getNonOpaquePointerElementType();
    }

    if (TargetTy->isStructTy()) {
      auto* Struct = dyn_cast<StructType>(TargetTy);
      return runOnStructRead(*Load.getParent(), Load, *GEP, *Struct);
    }
    /* Catch this kind of instruction:
     * `getelementptr i8, i8* %26, i32 4, !dbg !2963`
     */
    if (Target->getType()->isPointerTy() && TargetTy->isIntegerTy()) {
      SmallVector<Value*, 2> indices(GEP->idx_begin(), GEP->idx_end());
      if (indices.size() == 1 && isa<ConstantInt>(indices[0])) {
        if (TargetTy->getPrimitiveSizeInBits() == 8) {
          return runOnBufferRead(*Load.getParent(), Load, *GEP, *dyn_cast<ConstantInt>(indices[0]));
        } else {
          if (hasObf(Load, MObfTy::PROTECT_FIELD_ACCESS)) {
            // TODO(romain): To implement
            SWARN("[{}] Load instruction {} won't be protected", name(),
                  ToString(Load));
          }
        }
      }
    }

    if (TargetTy->isArrayTy()) {
      // TODO(romain): To implement
      SDEBUG("[{}] ArrayTy is not supported yet: {}", name(), ToString(Load));
      return false;
    }
  }

  if (auto* CE = dyn_cast<ConstantExpr>(LoadOp)) {
    return runOnConstantExprRead(*Load.getParent(), Load, *CE);
  }
  return false;
}

bool OpaqueFieldAccess::runOnConstantExprWrite(llvm::BasicBlock& BB, llvm::StoreInst& Store,
                                               llvm::ConstantExpr& CE)
{

  if (CE.getNumOperands() < 3) {
    return false;
  }

  auto* GV       = dyn_cast<GlobalVariable>(CE.getOperand(0));
  auto* OpOffset = dyn_cast<ConstantInt>(CE.getOperand(2));
  if (GV == nullptr || OpOffset == nullptr) {
    return false;
  }
  Type *GVTy = GV->getValueType();
  if (!GVTy->isArrayTy()) {
    SDEBUG("[{}] Global variable {} won't be processed: non Array type", name(),
           GV->getName());
    return false;
  }

  Type* ElTy = GVTy->getArrayElementType();

  PyConfig& conf = PyConfig::instance();
  if (!conf.getUserConfig()->obfuscate_variable_access(BB.getModule(), BB.getParent(), GV)) {
    return false;
  }

  SINFO("[{}] Executing on module {} over variable {}", name(),
        BB.getModule()->getName(), GV->getName());

  IRBuilder<NoFolder> IRB(&Store);
  Value* opaqueOffset =
    IRB.CreateOr(
      ConstantInt::get(IRB.getInt32Ty(), OpOffset->getLimitedValue()),
      ConstantInt::get(IRB.getInt32Ty(), 0));


  if (auto* Op = dyn_cast<Instruction>(opaqueOffset)) {
    addMetadata(*Op, {MetaObf(OPAQUE_CST), MetaObf(OPAQUE_OP, 2llu)});
  }

  Value* newGEP = IRB.CreateInBoundsGEP(ElTy,
                                        IRB.CreateBitCast(GV, ElTy->getPointerTo()),
                                        opaqueOffset);
  Store.setOperand(1, newGEP);
  return true;
}

bool OpaqueFieldAccess::runOnStructWrite(BasicBlock& BB, StoreInst& Store,
                                         GetElementPtrInst& GEP, StructType& S)
{
  PyConfig& conf = PyConfig::instance();
  if (!conf.getUserConfig()->obfuscate_struct_access(BB.getModule(), BB.getParent(), &S)) {
    return false;
  }

  SINFO("[{}] Executing on module {} over structure {}", name(),
        BB.getModule()->getName(), S.getName());

  SmallVector<Value*, 2> indices(GEP.idx_begin(), GEP.idx_end());

  if (indices.size() != 2) {
    SWARN("[{}] Store {} expects 2 indices for the structure {}", name(),
          ToString(Store), S.getName());
    return false;
  }

  Value* Idx0 = indices[0];
  Value* Idx1 = indices[1];

  auto* ZeroVal = dyn_cast<ConstantInt>(Idx0);
  auto* OffVal  = dyn_cast<ConstantInt>(Idx1);

  if (ZeroVal == nullptr || OffVal == nullptr) {
    return false;
  }

  if (ZeroVal->getLimitedValue() != 0) {
    SWARN("[{}] Expecting a zero value for the getelementptr {}", name(),
          ToString(GEP));
  }

  const DataLayout& DL = BB.getParent()->getParent()->getDataLayout();
  uint64_t ComputedOffset = DL.getStructLayout(&S)->getElementOffset(OffVal->getLimitedValue());

  SDEBUG("[{}] Obfuscating field WRITE access on {}->#{} (offset: {})", name(),
         S.getName(), OffVal->getLimitedValue(), ComputedOffset);

  IRBuilder<NoFolder> IRB(&Store);

  Value* opaqueOffset =
    IRB.CreateAdd(
      ConstantInt::get(IRB.getInt32Ty(), 0),
      ConstantInt::get(IRB.getInt32Ty(), ComputedOffset));

  if (auto* OpAdd = dyn_cast<Instruction>(opaqueOffset)) {
    addMetadata(*OpAdd, {MetaObf(OPAQUE_CST), MetaObf(OPAQUE_OP, 2llu)});
  }

  Value* newGEP =
    IRB.CreateBitCast(
      IRB.CreateInBoundsGEP(IRB.getInt8Ty(),
                            IRB.CreateBitCast(GEP.getPointerOperand(), IRB.getInt8PtrTy()),
                            opaqueOffset),
      GEP.getResultElementType()->getPointerTo());

  GEP.replaceAllUsesWith(newGEP);
  return true;
}

bool OpaqueFieldAccess::runOnBufferWrite(BasicBlock& BB, StoreInst& Store,
                                         GetElementPtrInst& GEP, ConstantInt& CI)
{
  if (!hasObf(Store, MObfTy::PROTECT_FIELD_ACCESS)) {
    return false;
  }
  SDEBUG("[{}] Obfuscating buffer write access", name());
  IRBuilder<NoFolder> IRB(&Store);
  uint64_t Val = CI.getLimitedValue();
  uint32_t lhs, rhs = 0;
  if (Val % 2 == 0) {
    lhs = Val / 2;
    rhs = Val / 2;
  } else {
    lhs = (Val - 1) / 2;
    rhs = (Val + 1) / 2;
  }

  Value* opaqueOffset =
    IRB.CreateAdd(
      ConstantInt::get(IRB.getInt32Ty(), lhs),
      ConstantInt::get(IRB.getInt32Ty(), rhs));

  if (auto* Op = dyn_cast<Instruction>(opaqueOffset)) {
    addMetadata(*Op, {MetaObf(OPAQUE_CST), MetaObf(OPAQUE_OP, 2llu)});
  }

  Value* newGEP =
    IRB.CreateBitCast(
      IRB.CreateInBoundsGEP(IRB.getInt8Ty(),
                            IRB.CreateBitCast(GEP.getPointerOperand(), IRB.getInt8PtrTy()),
                            opaqueOffset),
      GEP.getResultElementType()->getPointerTo());

  GEP.replaceAllUsesWith(newGEP);
  return true;
}

bool OpaqueFieldAccess::runOnStore(StoreInst& Store) {
  Value* StoreOp = Store.getPointerOperand();

  if (auto* GEP = dyn_cast<GetElementPtrInst>(StoreOp)) {
    Value* Target  = GEP->getPointerOperand();
    Type* TargetTy = Target->getType();

    if (TargetTy->isPointerTy() && !TargetTy->isOpaquePointerTy()) {
      TargetTy = TargetTy->getNonOpaquePointerElementType();
    }

    if (TargetTy->isStructTy()) {
      auto* Struct = dyn_cast<StructType>(TargetTy);
      return runOnStructWrite(*Store.getParent(), Store, *GEP, *Struct);
    }

    /* Catch this kind of instruction:
     * `getelementptr i8, i8* %26, i32 4, !dbg !2963`
     */
    if (Target->getType()->isPointerTy() && TargetTy->isIntegerTy()) {
      SmallVector<Value*, 2> indices(GEP->idx_begin(), GEP->idx_end());
      if (indices.size() == 1 && isa<ConstantInt>(indices[0])) {
        if (TargetTy->getPrimitiveSizeInBits() == 8) {
          return runOnBufferWrite(*Store.getParent(), Store, *GEP, *dyn_cast<ConstantInt>(indices[0]));
        } else {
          if (hasObf(Store, MObfTy::PROTECT_FIELD_ACCESS)) {
            // TODO(romain): To implement
            SWARN("[{}] Store instruction {} won't be protected", name(),
                  ToString(Store));
          }
        }
      }
      return false;
    }

    if (TargetTy->isArrayTy()) {
      // TODO(romain): To implement
      SDEBUG("[{}] ArrayTy is not supported yet for store {}", name(),
             ToString(Store));
      return false;
    }

    return false;
  }

  if (auto* CE = dyn_cast<ConstantExpr>(StoreOp)) {
    return runOnConstantExprWrite(*Store.getParent(), Store, *CE);
  }
  return false;
}

bool OpaqueFieldAccess::runOnBasicBlock(BasicBlock &BB) {
  bool Changed = false;
  for (Instruction& Inst : BB) {
    if (auto* Load = dyn_cast<LoadInst>(&Inst)) {
      Changed |= runOnLoad(*Load);
    }
    else if (auto* Store = dyn_cast<StoreInst>(&Inst)) {
      Changed |= runOnStore(*Store);
    }
  }
  return Changed;
}

PreservedAnalyses OpaqueFieldAccess::run(Module &M,
                                         ModuleAnalysisManager &FAM) {
  bool Changed = false;
  //dump(M, "/tmp/a.ll");
  for (Function& F : M) {
    for (BasicBlock& BB : F) {
      Changed |= runOnBasicBlock(BB);
    }
  }

  SINFO("[{}] Changes{}applied on module {}", name(), Changed ? " " : " not ",
        M.getName());

  return Changed ? PreservedAnalyses::none() :
                   PreservedAnalyses::all();

}
}


//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/Metadata.hpp"
#include "omvll/passes/opaque-constants/OpaqueConstants.hpp"
#include "omvll/passes/opaque-field-access/OpaqueFieldAccess.hpp"
#include "omvll/utils.hpp"

using namespace llvm;

namespace omvll {

bool OpaqueFieldAccess::runOnStructRead(BasicBlock &BB, LoadInst &Load,
                                        GetElementPtrInst &GEP, StructType &S) {
  PyConfig &Config = PyConfig::instance();
  if (!Config.getUserConfig()->obfuscateStructAccess(BB.getModule(),
                                                     BB.getParent(), &S))
    return false;

  SINFO("[{}] Executing on module {} over structure {}", name(),
        BB.getModule()->getName(), S.getName());

  SmallVector<Value *, 2> Indices(GEP.idx_begin(), GEP.idx_end());
  if (Indices.size() != 2) {
    SWARN("[{}] Load {} expects 2 indices for the structure {}", name(),
          ToString(Load), S.getName());
    return false;
  }

  auto *ZeroVal = dyn_cast<ConstantInt>(Indices[0]);
  auto *OffVal = dyn_cast<ConstantInt>(Indices[1]);
  if (!ZeroVal || !OffVal)
    return false;

  if (ZeroVal->getLimitedValue() != 0)
    SWARN("[{}] Expecting a zero value for the getelementptr: {}", name(),
          ToString(GEP));

  const DataLayout &DL = BB.getParent()->getParent()->getDataLayout();
  uint64_t ComputedOffset = DL.getStructLayout(&S)->getElementOffset(OffVal->getLimitedValue());

  SDEBUG("[{}] Obfuscating field READ access on {}->#{} (offset: {})", name(),
         S.getName(), OffVal->getLimitedValue(), ComputedOffset);

  IRBuilder<NoFolder> IRB(&Load);
  Value *OpaqueOffset =
      IRB.CreateAdd(ConstantInt::get(IRB.getInt32Ty(), 0),
                    ConstantInt::get(IRB.getInt32Ty(), ComputedOffset));

  if (auto *OpAdd = dyn_cast<Instruction>(OpaqueOffset))
    addMetadata(*OpAdd, {MetaObf(OpaqueCst), MetaObf(OpaqueOp, 2LLU)});

  Value *NewGEP = IRB.CreateBitCast(
      IRB.CreateInBoundsGEP(
          IRB.getInt8Ty(),
          IRB.CreateBitCast(GEP.getPointerOperand(), IRB.getInt8PtrTy()),
          OpaqueOffset),
      GEP.getResultElementType()->getPointerTo());

  GEP.replaceAllUsesWith(NewGEP);
  return true;
}

bool OpaqueFieldAccess::runOnBufferRead(BasicBlock &BB, LoadInst &Load,
                                        GetElementPtrInst &GEP,
                                        ConstantInt &CI) {
  if (!hasObf(Load, MetaObfTy::ProtectFieldAccess))
    return false;

  SDEBUG("[{}] Obfuscating buffer load access", name());
  IRBuilder<NoFolder> IRB(&Load);
  const uint64_t Val = CI.getLimitedValue();
  uint32_t Lhs, Rhs = 0;

  if (Val % 2 == 0) {
    Lhs = Val / 2;
    Rhs = Val / 2;
  } else {
    Lhs = (Val - 1) / 2;
    Rhs = (Val + 1) / 2;
  }

  Value *OpaqueOffset = IRB.CreateAdd(ConstantInt::get(IRB.getInt32Ty(), Lhs),
                                      ConstantInt::get(IRB.getInt32Ty(), Rhs));

  if (auto *Op = dyn_cast<Instruction>(OpaqueOffset))
    addMetadata(*Op, {MetaObf(OpaqueCst), MetaObf(OpaqueOp, 2LLU)});

  Value *NewGEP = IRB.CreateBitCast(
      IRB.CreateInBoundsGEP(
          IRB.getInt8Ty(),
          IRB.CreateBitCast(GEP.getPointerOperand(), IRB.getInt8PtrTy()),
          OpaqueOffset),
      GEP.getResultElementType()->getPointerTo());

  GEP.replaceAllUsesWith(NewGEP);
  return true;
}

bool OpaqueFieldAccess::runOnConstantExprRead(BasicBlock &BB, LoadInst &Load,
                                              ConstantExpr &CE) {
  if (CE.getNumOperands() < 3)
    return false;

  auto *GV = dyn_cast<GlobalVariable>(CE.getOperand(0));
  auto *OpOffset = dyn_cast<ConstantInt>(CE.getOperand(2));
  if (!GV || !OpOffset)
    return false;

  Type *GVTy = GV->getValueType();
  if (!GVTy->isArrayTy()) {
    SDEBUG("[{}] Global variable {} will not be processed: non array type",
           name(), GV->getName());
    return false;
  }

  Type *ElTy = GVTy->getArrayElementType();
  PyConfig &Config = PyConfig::instance();
  if (!Config.getUserConfig()->obfuscateVariableAccess(BB.getModule(),
                                                       BB.getParent(), GV))
    return false;

  SINFO("[{}] Executing on module {} over variable {}", name(),
        BB.getModule()->getName(), GV->getName());

  IRBuilder<NoFolder> IRB(&Load);
  Value *OpaqueOffset = IRB.CreateSub(
      ConstantInt::get(IRB.getInt32Ty(), OpOffset->getLimitedValue()),
      ConstantInt::get(IRB.getInt32Ty(), 0));

  if (auto *Op = dyn_cast<Instruction>(OpaqueOffset))
    addMetadata(*Op, {MetaObf(OpaqueCst), MetaObf(OpaqueOp, 2LLU)});

  Value *NewGEP = IRB.CreateInBoundsGEP(
      ElTy, IRB.CreateBitCast(GV, ElTy->getPointerTo()), OpaqueOffset);
  Load.setOperand(0, NewGEP);
  return true;
}

bool OpaqueFieldAccess::runOnLoad(LoadInst &Load) {
  Value *LoadOp = Load.getPointerOperand();

  if (auto *GEP = dyn_cast<GetElementPtrInst>(LoadOp)) {
    Value *Target = GEP->getPointerOperand();
    Type *TargetTy = GEP->getSourceElementType();

    if (TargetTy->isStructTy()) {
      auto *Struct = dyn_cast<StructType>(TargetTy);
      return runOnStructRead(*Load.getParent(), Load, *GEP, *Struct);
    }

    // Catch this instruction: `getelementptr i8, i8* %26, i32 4, !dbg !2963`
    if (Target->getType()->isPointerTy() && TargetTy->isIntegerTy()) {
      SmallVector<Value *, 2> Indices(GEP->idx_begin(), GEP->idx_end());
      if (Indices.size() == 1 && isa<ConstantInt>(Indices[0])) {
        if (TargetTy->getPrimitiveSizeInBits() == 8) {
          return runOnBufferRead(*Load.getParent(), Load, *GEP,
                                 *cast<ConstantInt>(Indices[0]));
        } else {
          if (hasObf(Load, MetaObfTy::ProtectFieldAccess)) {
            // TODO(romain): To implement.
            SWARN("[{}] Load instruction {} will not be protected", name(),
                  ToString(Load));
          }
        }
      }
    }

    if (TargetTy->isArrayTy()) {
      // TODO(romain): To implement.
      SDEBUG("[{}] ArrayTy is not supported yet: {}", name(), ToString(Load));
      return false;
    }
  }

  if (auto *CE = dyn_cast<ConstantExpr>(LoadOp))
    return runOnConstantExprRead(*Load.getParent(), Load, *CE);
  return false;
}

bool OpaqueFieldAccess::runOnConstantExprWrite(BasicBlock &BB, StoreInst &Store,
                                               ConstantExpr &CE) {
  if (CE.getNumOperands() < 3)
    return false;

  auto *GV = dyn_cast<GlobalVariable>(CE.getOperand(0));
  auto *OpOffset = dyn_cast<ConstantInt>(CE.getOperand(2));
  if (!GV || !OpOffset)
    return false;

  Type *GVTy = GV->getValueType();
  if (!GVTy->isArrayTy()) {
    SDEBUG("[{}] Global variable {} will not be processed: non Array type",
           name(), GV->getName());

    return false;
  }

  Type *ElTy = GVTy->getArrayElementType();
  PyConfig &Config = PyConfig::instance();
  if (!Config.getUserConfig()->obfuscateVariableAccess(BB.getModule(),
                                                       BB.getParent(), GV))
    return false;

  SINFO("[{}] Executing on module {} over variable {}", name(),
        BB.getModule()->getName(), GV->getName());

  IRBuilder<NoFolder> IRB(&Store);
  Value *OpaqueOffset = IRB.CreateOr(
      ConstantInt::get(IRB.getInt32Ty(), OpOffset->getLimitedValue()),
      ConstantInt::get(IRB.getInt32Ty(), 0));

  if (auto *Op = dyn_cast<Instruction>(OpaqueOffset))
    addMetadata(*Op, {MetaObf(OpaqueCst), MetaObf(OpaqueOp, 2LLU)});

  Value *NewGEP = IRB.CreateInBoundsGEP(
      ElTy, IRB.CreateBitCast(GV, ElTy->getPointerTo()), OpaqueOffset);
  Store.setOperand(1, NewGEP);
  return true;
}

bool OpaqueFieldAccess::runOnStructWrite(BasicBlock &BB, StoreInst &Store,
                                         GetElementPtrInst &GEP,
                                         StructType &S) {
  PyConfig &Config = PyConfig::instance();
  if (!Config.getUserConfig()->obfuscateStructAccess(BB.getModule(),
                                                     BB.getParent(), &S))
    return false;

  SINFO("[{}] Executing on module {} over structure {}", name(),
        BB.getModule()->getName(), S.getName());

  SmallVector<Value *, 2> Indices(GEP.idx_begin(), GEP.idx_end());
  if (Indices.size() != 2) {
    SWARN("[{}] Store {} expects 2 indices for the structure {}", name(),
          ToString(Store), S.getName());
    return false;
  }

  auto *ZeroVal = dyn_cast<ConstantInt>(Indices[0]);
  auto *OffVal = dyn_cast<ConstantInt>(Indices[1]);
  if (!ZeroVal || !OffVal)
    return false;

  if (ZeroVal->getLimitedValue() != 0)
    SWARN("[{}] Expecting a zero value for the getelementptr {}", name(),
          ToString(GEP));

  const DataLayout &DL = BB.getParent()->getParent()->getDataLayout();
  uint64_t ComputedOffset = DL.getStructLayout(&S)->getElementOffset(OffVal->getLimitedValue());

  SDEBUG("[{}] Obfuscating field WRITE access on {}->#{} (offset: {})", name(),
         S.getName(), OffVal->getLimitedValue(), ComputedOffset);

  IRBuilder<NoFolder> IRB(&Store);

  Value *OpaqueOffset =
      IRB.CreateAdd(ConstantInt::get(IRB.getInt32Ty(), 0),
                    ConstantInt::get(IRB.getInt32Ty(), ComputedOffset));

  if (auto *OpAdd = dyn_cast<Instruction>(OpaqueOffset))
    addMetadata(*OpAdd, {MetaObf(OpaqueCst), MetaObf(OpaqueOp, 2LLU)});

  Value *NewGEP = IRB.CreateBitCast(
      IRB.CreateInBoundsGEP(
          IRB.getInt8Ty(),
          IRB.CreateBitCast(GEP.getPointerOperand(), IRB.getInt8PtrTy()),
          OpaqueOffset),
      GEP.getResultElementType()->getPointerTo());

  GEP.replaceAllUsesWith(NewGEP);
  return true;
}

bool OpaqueFieldAccess::runOnBufferWrite(BasicBlock &BB, StoreInst &Store,
                                         GetElementPtrInst &GEP,
                                         ConstantInt &CI) {
  if (!hasObf(Store, MetaObfTy::ProtectFieldAccess))
    return false;

  SDEBUG("[{}] Obfuscating buffer write access", name());

  IRBuilder<NoFolder> IRB(&Store);
  uint64_t Val = CI.getLimitedValue();
  uint32_t Lhs, Rhs = 0;

  if (Val % 2 == 0) {
    Lhs = Val / 2;
    Rhs = Val / 2;
  } else {
    Lhs = (Val - 1) / 2;
    Rhs = (Val + 1) / 2;
  }

  Value *OpaqueOffset = IRB.CreateAdd(ConstantInt::get(IRB.getInt32Ty(), Lhs),
                                      ConstantInt::get(IRB.getInt32Ty(), Rhs));

  if (auto *Op = dyn_cast<Instruction>(OpaqueOffset))
    addMetadata(*Op, {MetaObf(OpaqueCst), MetaObf(OpaqueOp, 2LLU)});

  Value *NewGEP = IRB.CreateBitCast(
      IRB.CreateInBoundsGEP(
          IRB.getInt8Ty(),
          IRB.CreateBitCast(GEP.getPointerOperand(), IRB.getInt8PtrTy()),
          OpaqueOffset),
      GEP.getResultElementType()->getPointerTo());

  GEP.replaceAllUsesWith(NewGEP);
  return true;
}

bool OpaqueFieldAccess::runOnStore(StoreInst &Store) {
  Value *StoreOp = Store.getPointerOperand();

  if (auto *GEP = dyn_cast<GetElementPtrInst>(StoreOp)) {
    Value *Target = GEP->getPointerOperand();
    Type *TargetTy = GEP->getSourceElementType();

    if (TargetTy->isStructTy()) {
      auto *Struct = dyn_cast<StructType>(TargetTy);
      return runOnStructWrite(*Store.getParent(), Store, *GEP, *Struct);
    }

    // Catch this instruction: `getelementptr i8, i8* %26, i32 4, !dbg !2963`
    if (Target->getType()->isPointerTy() && TargetTy->isIntegerTy()) {
      SmallVector<Value *, 2> Indices(GEP->idx_begin(), GEP->idx_end());
      if (Indices.size() == 1 && isa<ConstantInt>(Indices[0])) {
        if (TargetTy->getPrimitiveSizeInBits() == 8) {
          return runOnBufferWrite(*Store.getParent(), Store, *GEP,
                                  *cast<ConstantInt>(Indices[0]));
        } else {
          if (hasObf(Store, MetaObfTy::ProtectFieldAccess)) {
            // TODO(romain): To implement.
            SWARN("[{}] Store instruction {} will not be protected", name(),
                  ToString(Store));
          }
        }
      }
      return false;
    }

    if (TargetTy->isArrayTy()) {
      // TODO(romain): To implement.
      SDEBUG("[{}] ArrayTy is not supported yet for store {}", name(),
             ToString(Store));
      return false;
    }

    return false;
  }

  if (auto *CE = dyn_cast<ConstantExpr>(StoreOp))
    return runOnConstantExprWrite(*Store.getParent(), Store, *CE);
  return false;
}

bool OpaqueFieldAccess::runOnBasicBlock(BasicBlock &BB) {
  bool Changed = false;
  for (Instruction &Inst : BB) {
    if (auto *Load = dyn_cast<LoadInst>(&Inst))
      Changed |= runOnLoad(*Load);
    else if (auto *Store = dyn_cast<StoreInst>(&Inst))
      Changed |= runOnStore(*Store);
  }
  return Changed;
}

PreservedAnalyses OpaqueFieldAccess::run(Module &M,
                                         ModuleAnalysisManager &FAM) {
  bool Changed = false;
  for (Function &F : M)
    for (BasicBlock &BB : F)
      Changed |= runOnBasicBlock(BB);

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll

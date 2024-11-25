#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/PassManager.h"

// Forward declarations
namespace llvm {
class UnaryInstruction;
class StructType;
class GetElementPtrInst;
class LoadInst;
class StoreInst;
class ConstantInt;
class ConstantExpr;
} // end namespace llvm

namespace omvll {

// See https://obfuscator.re/omvll/passes/opaque-fields-access for details.
struct OpaqueFieldAccess : llvm::PassInfoMixin<OpaqueFieldAccess> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
  bool shouldRun(llvm::BasicBlock &BB);

  bool runOnBasicBlock(llvm::BasicBlock &BB);

  bool runOnLoad(llvm::LoadInst &Load);

  bool runOnStructRead(llvm::BasicBlock &BB, llvm::LoadInst &Load,
                       llvm::GetElementPtrInst &GEP, llvm::StructType &S);

  bool runOnBufferRead(llvm::BasicBlock &BB, llvm::LoadInst &Load,
                       llvm::GetElementPtrInst &GEP, llvm::ConstantInt &CI);

  bool runOnConstantExprRead(llvm::BasicBlock &BB, llvm::LoadInst &Load,
                             llvm::ConstantExpr &CE);

  bool runOnStore(llvm::StoreInst &Store);

  bool runOnStructWrite(llvm::BasicBlock &BB, llvm::StoreInst &Store,
                        llvm::GetElementPtrInst &GEP, llvm::StructType &S);

  bool runOnBufferWrite(llvm::BasicBlock &BB, llvm::StoreInst &Store,
                        llvm::GetElementPtrInst &GEP, llvm::ConstantInt &CI);

  bool runOnConstantExprWrite(llvm::BasicBlock &BB, llvm::StoreInst &Store,
                              llvm::ConstantExpr &CE);
};

} // end namespace omvll

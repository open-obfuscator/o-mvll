#ifndef OMVLL_OPAQUE_FIELD_ACCESS_H
#define OMVLL_OPAQUE_FIELD_ACCESS_H
#include "llvm/IR/PassManager.h"

namespace llvm {
class UnaryInstruction;
class StructType;
class GetElementPtrInst;
class LoadInst;
class StoreInst;
class ConstantInt;
class ConstantExpr;
}

namespace omvll {

//! See: https://obfuscator.re/omvll/passes/opaque-fields-access for the details
struct OpaqueFieldAccess : llvm::PassInfoMixin<OpaqueFieldAccess> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
  bool runOnBasicBlock(llvm::BasicBlock &BB);
  bool shouldRun(llvm::BasicBlock& BB);

  bool runOnLoad(llvm::LoadInst& Load);

  bool runOnStructRead(llvm::BasicBlock& BB, llvm::LoadInst& Load,
                       llvm::GetElementPtrInst& GEP, llvm::StructType& S);

  bool runOnBufferRead(llvm::BasicBlock& BB, llvm::LoadInst& Load,
                       llvm::GetElementPtrInst& GEP, llvm::ConstantInt& CI);

  bool runOnConstantExprRead(llvm::BasicBlock& BB, llvm::LoadInst& Load,
                             llvm::ConstantExpr& CE);

  bool runOnStore(llvm::StoreInst& Store);

  bool runOnStructWrite(llvm::BasicBlock& BB, llvm::StoreInst& Store,
                        llvm::GetElementPtrInst& GEP, llvm::StructType& S);

  bool runOnBufferWrite(llvm::BasicBlock& BB, llvm::StoreInst& Store,
                        llvm::GetElementPtrInst& GEP, llvm::ConstantInt& CI);

  bool runOnConstantExprWrite(llvm::BasicBlock& BB, llvm::StoreInst& Store,
                              llvm::ConstantExpr& CE);

};
}

#endif

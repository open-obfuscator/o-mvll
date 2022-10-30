#ifndef OMVLL_OPAQUE_CONSTANTS_H
#define OMVLL_OPAQUE_CONSTANTS_H
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/DenseMap.h"

#include "omvll/passes/opaque-constants/OpaqueConstantsOpt.hpp"

namespace llvm {
class Instruction;
class ConstantInt;
class Value;
class Use;
class AllocaInst;
class Type;
}

namespace omvll {
struct OpaqueContext {
  llvm::AllocaInst* T1;
  llvm::AllocaInst* T2;
};

// See: https://obfuscator.re/omvll/passes/opaque-constants for the details
struct OpaqueConstants : llvm::PassInfoMixin<OpaqueConstants> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
  bool runOnBasicBlock(llvm::BasicBlock &BB, OpaqueConstantsOpt* opt);
  bool Process(llvm::Instruction& I, OpaqueConstantsOpt* opt);
  bool Process(llvm::Instruction& I, llvm::Use& Op, llvm::ConstantInt& CI, OpaqueConstantsOpt* opt);

  OpaqueContext* getOrCreateContext(llvm::BasicBlock& BB);

  llvm::Value* GetOpaqueZero(llvm::Instruction& I, OpaqueContext& C, llvm::Type* Ty);
  llvm::Value* GetOpaqueOne(llvm::Instruction& I, OpaqueContext& C, llvm::Type* Ty);
  llvm::Value* GetOpaqueCst(llvm::Instruction& I, OpaqueContext& C, const llvm::ConstantInt& Val);

  private:
  std::unique_ptr<llvm::RandomNumberGenerator> RNG_;
  llvm::DenseMap<llvm::BasicBlock*, OpaqueContext> ctx_;
  llvm::DenseMap<llvm::Function*, OpaqueConstantsOpt> opts_;

};
}

#endif

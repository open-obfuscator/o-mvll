#ifndef OMVLL_CFG_FLAT_H
#define OMVLL_CFG_FLAT_H
#include "llvm/IR/PassManager.h"

namespace omvll {

// The classical control-flow flattening pass
// See: https://obfuscator.re/omvll/passes/control-flow-flattening/ for the details
struct ControlFlowFlattening : llvm::PassInfoMixin<ControlFlowFlattening> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
  bool runOnFunction(llvm::Function &F, llvm::RandomNumberGenerator& RNG);
};
}

#endif

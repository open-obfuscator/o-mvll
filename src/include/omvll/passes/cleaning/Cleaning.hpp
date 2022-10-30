#ifndef OMVLL_CLEANING_H
#define OMVLL_CLEANING_H
#include "llvm/IR/PassManager.h"

namespace llvm {
}

namespace omvll {

struct Cleaning : llvm::PassInfoMixin<Cleaning> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
};
}

#endif

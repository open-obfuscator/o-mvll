#ifndef OMVLL_TAKINJECTION_H
#define OMVLL_TAKINJECTION_H
#include "llvm/IR/PassManager.h"

namespace omvll {

struct TakInjection : llvm::PassInfoMixin<TakInjection> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
};
} // namespace omvll

#endif

#ifndef OMVLL_OBJCLEANER_H
#define OMVLL_OBJCLEANER_H
#include "llvm/IR/PassManager.h"

namespace llvm {
class GlobalVariable;
}

namespace omvll {

/*
 * /!\ THIS PASS IS NOT YET FULLY IMPLEMENTED /!\
 *
 * This pass aims at mangling/obfuscating objective-C symbols
 */
struct ObjCleaner : llvm::PassInfoMixin<ObjCleaner> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
  bool runGlobalVariable(llvm::Function &F);
};
}

#endif

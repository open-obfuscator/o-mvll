#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/PassManager.h"

// Forward declarations
namespace llvm {
class GlobalVariable;
} // end namespace llvm

namespace omvll {

// This pass aims at mangling/obfuscating Objective-C symbols.
// Note: this pass is incomplete.
struct ObjCleaner : llvm::PassInfoMixin<ObjCleaner> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
  bool runGlobalVariable(llvm::Function &F);
};

} // end namespace omvll

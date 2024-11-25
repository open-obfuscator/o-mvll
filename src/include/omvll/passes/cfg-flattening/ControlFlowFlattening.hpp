#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/PassManager.h"

namespace omvll {

// The classical control-flow flattening pass.
// See https://obfuscator.re/omvll/passes/control-flow-flattening/ for details.
struct ControlFlowFlattening : llvm::PassInfoMixin<ControlFlowFlattening> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
  bool runOnFunction(llvm::Function &F, llvm::RandomNumberGenerator &RNG);
};

} // end namespace omvll

#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/PassManager.h"

namespace omvll {

struct FunctionOutline : llvm::PassInfoMixin<FunctionOutline> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);

  bool process(llvm::Function &F, llvm::LLVMContext &Ctx, unsigned);

private:
  std::unique_ptr<llvm::RandomNumberGenerator> RNG;
};

} // end namespace omvll

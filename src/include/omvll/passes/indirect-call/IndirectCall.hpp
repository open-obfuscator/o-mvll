#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/PassManager.h"

namespace omvll {

struct IndirectCall : llvm::PassInfoMixin<IndirectCall> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);

  bool process(llvm::Function &F, const llvm::DataLayout &DL,
               llvm::LLVMContext &Ctx);

private:
  std::unique_ptr<llvm::RandomNumberGenerator> RNG;
};

} // end namespace omvll

#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/PassManager.h"

// Forward declarations
namespace llvm {
class Function;
class RandomNumberGenerator;
class BasicBlock;
} // end namespace llvm

namespace omvll {

class Jitter;

// See https://obfuscator.re/omvll/passes/control-flow-breaking/ for details.
struct BreakControlFlow : llvm::PassInfoMixin<BreakControlFlow> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);

  bool runOnEntryBlock(llvm::Function &F, llvm::BasicBlock &EntryBlock);
  bool runOnTerminators(llvm::Function &F, llvm::BasicBlock &TBlock);
  bool runOnFunction(llvm::Function &F);

  std::unique_ptr<Jitter> JIT;
  std::unique_ptr<llvm::RandomNumberGenerator> RNG;
};

} // end namespace omvll

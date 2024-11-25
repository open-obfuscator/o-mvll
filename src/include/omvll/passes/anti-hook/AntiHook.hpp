#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/PassManager.h"
#include "llvm/Support/RandomNumberGenerator.h"

// Forward declarations
namespace llvm {
class Function;
} // end namespace llvm

namespace omvll {

class Jitter;

// Frida Anti-Hooking.
// See https://obfuscator.re/omvll/passes/anti-hook/ for details.
struct AntiHook : llvm::PassInfoMixin<AntiHook> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);

  bool runOnFunction(llvm::Function &F);

private:
  std::unique_ptr<llvm::RandomNumberGenerator> RNG;
  std::unique_ptr<Jitter> JIT;
};

} // end namespace omvll

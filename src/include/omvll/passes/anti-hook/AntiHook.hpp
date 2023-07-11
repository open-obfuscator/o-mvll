#ifndef OMVLL_ANTI_HOOK_H
#define OMVLL_ANTI_HOOK_H
#include "llvm/IR/PassManager.h"
#include "llvm/Support/RandomNumberGenerator.h"

namespace llvm {
class Function;
}

namespace omvll {
class Jitter;

//! Frida anti-hooking
//! See: https://obfuscator.re/omvll/passes/anti-hook/ for the details
struct AntiHook : llvm::PassInfoMixin<AntiHook> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);

  bool runOnFunction(llvm::Function &F);

private:
  std::unique_ptr<llvm::RandomNumberGenerator> RNG_;
  std::unique_ptr<Jitter> jitter_;
};
}

#endif

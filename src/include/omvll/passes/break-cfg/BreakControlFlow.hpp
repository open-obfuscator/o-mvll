#ifndef OMVLL_BREAK_CONTROL_FLOW_H
#define OMVLL_BREAK_CONTROL_FLOW_H
#include "llvm/IR/PassManager.h"

namespace llvm {
class Function;
class RandomNumberGenerator;
class BasicBlock;
}

namespace omvll {
class Jitter;

// See: https://obfuscator.re/omvll/passes/control-flow-breaking/ for the details
struct BreakControlFlow : llvm::PassInfoMixin<BreakControlFlow> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
  bool runOnEntryBlock(llvm::Function &F, llvm::BasicBlock& EntryBlock);
  bool runOnTerminators(llvm::Function &F, llvm::BasicBlock& TBlock);
  bool runOnFunction(llvm::Function &F);

  std::unique_ptr<Jitter> Jitter_;
  std::unique_ptr<llvm::RandomNumberGenerator> RNG_;
};
}

#endif

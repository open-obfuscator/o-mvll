#ifndef OMVLL_ARITHMETIC_H
#define OMVLL_ARITHMETIC_H
#include "llvm/IR/PassManager.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/RandomNumberGenerator.h"

#include "omvll/passes/arithmetic/ArithmeticOpt.hpp"

namespace llvm {
class Function;
class Module;
class Value;
class BinaryOperator;
}

namespace omvll {

// See: https://obfuscator.re/omvll/passes/arithmetic/ for the details
struct Arithmetic : llvm::PassInfoMixin<Arithmetic> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
  bool runOnBasicBlock(llvm::BasicBlock &BB);

  llvm::Function* injectFunWrapper(llvm::Module& M, llvm::BinaryOperator& Op,
                                   llvm::Value& Lhs, llvm::Value& Rhs);

  static bool isSupported(const llvm::BinaryOperator& Op);
  private:
  llvm::DenseMap<llvm::Function*, ArithmeticOpt> opts_;
  std::unique_ptr<llvm::RandomNumberGenerator> RNG_;
};
}

#endif

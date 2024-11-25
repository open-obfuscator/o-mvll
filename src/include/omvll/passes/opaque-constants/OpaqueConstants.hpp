#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/PassManager.h"
#include "llvm/ADT/DenseMap.h"

#include "omvll/passes/opaque-constants/OpaqueConstantsOpt.hpp"

// Forward declarations
namespace llvm {
class Instruction;
class ConstantInt;
class Value;
class Use;
class AllocaInst;
class Type;
} // end namespace llvm

namespace omvll {

struct OpaqueContext {
  llvm::AllocaInst *T1;
  llvm::AllocaInst *T2;
};

// See https://obfuscator.re/omvll/passes/opaque-constants for details.
struct OpaqueConstants : llvm::PassInfoMixin<OpaqueConstants> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
  bool runOnBasicBlock(llvm::BasicBlock &BB, OpaqueConstantsOpt *Opt);

  bool process(llvm::Instruction &I, OpaqueConstantsOpt *Opt);
  bool process(llvm::Instruction &I, llvm::Use &Op, llvm::ConstantInt &CI,
               OpaqueConstantsOpt *Opt);

  OpaqueContext *getOrCreateContext(llvm::BasicBlock &BB);

  llvm::Value *getOpaqueZero(llvm::Instruction &I, OpaqueContext &C,
                             llvm::Type *Ty);
  llvm::Value *getOpaqueOne(llvm::Instruction &I, OpaqueContext &C,
                            llvm::Type *Ty);
  llvm::Value *getOpaqueCst(llvm::Instruction &I, OpaqueContext &C,
                            const llvm::ConstantInt &Val);

private:
  std::unique_ptr<llvm::RandomNumberGenerator> RNG;
  llvm::DenseMap<llvm::BasicBlock *, OpaqueContext> Ctx;
  llvm::DenseMap<llvm::Function *, OpaqueConstantsOpt> Opts;
};

} // end namespace omvll

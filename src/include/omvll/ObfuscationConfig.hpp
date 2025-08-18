#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include "omvll/passes/ObfuscationOpt.hpp"

// Forward declarations
namespace llvm {
class Module;
class Function;
class StructType;
class GlobalVariable;
} // end namespace llvm

namespace omvll {

struct ObfuscationConfig {
  ObfuscationConfig() = default;
  virtual ~ObfuscationConfig() = default;
  ObfuscationConfig(const ObfuscationConfig &) = delete;
  ObfuscationConfig &operator=(const ObfuscationConfig &) = delete;

  virtual StringEncodingOpt obfuscateString(llvm::Module *M, llvm::Function *F,
                                            const std::string &Str) = 0;

  virtual StructAccessOpt obfuscateStructAccess(llvm::Module *M,
                                                llvm::Function *F,
                                                llvm::StructType *S) = 0;

  virtual VarAccessOpt obfuscateVariableAccess(llvm::Module *M,
                                               llvm::Function *F,
                                               llvm::GlobalVariable *S) = 0;

  virtual BreakControlFlowOpt breakControlFlow(llvm::Module *M,
                                               llvm::Function *F) = 0;

  virtual ControlFlowFlatteningOpt
  controlFlowGraphFlattening(llvm::Module *M, llvm::Function *F) = 0;

  virtual OpaqueConstantsOpt obfuscateConstants(llvm::Module *M,
                                                llvm::Function *F) = 0;

  virtual ArithmeticOpt obfuscateArithmetics(llvm::Module *M,
                                             llvm::Function *F) = 0;

  virtual AntiHookOpt antiHooking(llvm::Module *M, llvm::Function *F) = 0;

  virtual IndirectBranchOpt indirectBranch(llvm::Module *M,
                                           llvm::Function *F) = 0;

  virtual IndirectCallOpt indirectCall(llvm::Module *M, llvm::Function *F) = 0;

  virtual BasicBlockDuplicateOpt basicBlockDuplicate(llvm::Module *M,
                                                     llvm::Function *F) = 0;

  virtual bool defaultConfig(llvm::Module *M, llvm::Function *F,
                             const std::vector<std::string> &ModuleExcludes,
                             const std::vector<std::string> &FunctionExcludes,
                             const std::vector<std::string> &FunctionIncludes,
                             int Probability) = 0;

  virtual bool hasReportDiffOverride() { return false; };
  virtual void reportDiff(const std::string &Pass, const std::string &Original,
                          const std::string &Obfuscated) = 0;
};

} // end namespace omvll

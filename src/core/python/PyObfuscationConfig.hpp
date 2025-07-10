#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <mutex>

#include "omvll/ObfuscationConfig.hpp"

namespace omvll {

class PyObfuscationConfig : public ObfuscationConfig {
  using ObfuscationConfig::ObfuscationConfig;

  StringEncodingOpt obfuscateString(llvm::Module *M, llvm::Function *F,
                                    const std::string &Str) override;

  BreakControlFlowOpt breakControlFlow(llvm::Module *M,
                                       llvm::Function *F) override;

  ControlFlowFlatteningOpt
  controlFlowGraphFlattening(llvm::Module *M, llvm::Function *F) override;

  StructAccessOpt obfuscateStructAccess(llvm::Module *M, llvm::Function *F,
                                        llvm::StructType *S) override;

  VarAccessOpt obfuscateVariableAccess(llvm::Module *M, llvm::Function *F,
                                       llvm::GlobalVariable *S) override;

  AntiHookOpt antiHooking(llvm::Module *M, llvm::Function *F) override;

  ArithmeticOpt obfuscateArithmetics(llvm::Module *M,
                                     llvm::Function *F) override;

  OpaqueConstantsOpt obfuscateConstants(llvm::Module *M,
                                        llvm::Function *F) override;

  IndirectBranchOpt indirectBranch(llvm::Module *M, llvm::Function *F) override;

  IndirectCallOpt indirectCall(llvm::Module *M, llvm::Function *F) override;

  bool defaultConfig(llvm::Module *M, llvm::Function *F,
                     const std::vector<std::string> &ModuleExcludes = {},
                     const std::vector<std::string> &FunctionExcludes = {},
                     const std::vector<std::string> &FunctionIncludes = {},
                     int Probability = 0) override;

  bool hasReportDiffOverride() override;
  void reportDiff(const std::string &Pass, const std::string &Original,
                  const std::string &Obfuscated) override;

private:
  bool OverridesReportDiff = false;
  std::once_flag OverridesReportDiffChecked;
};

} // end namespace omvll

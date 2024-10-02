#ifndef OMVLL_PY_OBFUSCATION_CONFIG_H
#define OMVLL_PY_OBFUSCATION_CONFIG_H
#include "omvll/ObfuscationConfig.hpp"
#include <mutex>

namespace omvll {

class PyObfuscationConfig : public ObfuscationConfig {
  using ObfuscationConfig::ObfuscationConfig;
  StringEncodingOpt obfuscate_string(llvm::Module* mod, llvm::Function* func,
                                     const std::string& str) override;

  BreakControlFlowOpt break_control_flow(llvm::Module* mod, llvm::Function* func) override;
  ControlFlowFlatteningOpt flatten_cfg(llvm::Module* mod, llvm::Function* func) override;

  StructAccessOpt obfuscate_struct_access(llvm::Module* mod, llvm::Function* func,
                                          llvm::StructType* S) override;
  VarAccessOpt obfuscate_variable_access(llvm::Module* mod, llvm::Function* func,
                                         llvm::GlobalVariable* S) override;
  AntiHookOpt anti_hooking(llvm::Module* mod, llvm::Function* func) override;
  ArithmeticOpt obfuscate_arithmetic(llvm::Module* mod, llvm::Function* func) override;
  OpaqueConstantsOpt obfuscate_constants(llvm::Module* mod, llvm::Function* func) override;

  bool has_report_diff_override() override;
  void report_diff(const std::string &pass, const std::string &original,
                   const std::string &obfuscated) override;

private:
  bool overrides_report_diff_ = false;
  std::once_flag overrides_report_diff_checked_;
};
}
#endif

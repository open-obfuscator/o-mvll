#ifndef OMVLL_OBF_CONFIG_H
#define OMVLL_OBF_CONFIG_H

#include "omvll/passes/ObfuscationOpt.hpp"

namespace llvm {
class Module;
class Function;
class StructType;
class GlobalVariable;
}


namespace omvll {
struct ObfuscationConfig {
  ObfuscationConfig() = default;
  virtual ~ObfuscationConfig() = default;
  ObfuscationConfig(const ObfuscationConfig&) = delete;
  ObfuscationConfig& operator=(const ObfuscationConfig&) = delete;

  virtual StringEncodingOpt obfuscate_string(llvm::Module* mod, llvm::Function* func,
                                             const std::string& str) = 0;

  virtual StructAccessOpt obfuscate_struct_access(llvm::Module* mod, llvm::Function* func,
                                                  llvm::StructType* S) = 0;

  virtual VarAccessOpt obfuscate_variable_access(llvm::Module* mod, llvm::Function* func,
                                                 llvm::GlobalVariable* S) = 0;

  virtual BreakControlFlowOpt break_control_flow(llvm::Module* mod, llvm::Function* func) = 0;

  virtual ControlFlowFlatteningOpt flatten_cfg(llvm::Module* mod, llvm::Function* func) = 0;

  virtual OpaqueConstantsOpt obfuscate_constants(llvm::Module* mod, llvm::Function* func) = 0;

  virtual ArithmeticOpt obfuscate_arithmetic(llvm::Module* mod, llvm::Function* func) = 0;

  virtual AntiHookOpt anti_hooking(llvm::Module* mod, llvm::Function* func) = 0;

  virtual bool has_report_diff_override() const { return false; };
  virtual void report_diff(const std::string &pass, const std::string &original,
                           const std::string &obfuscated) = 0;
};

}
#endif

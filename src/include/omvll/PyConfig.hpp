#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <string>
#include <memory>
#include <vector>

#include "omvll/passes/ObfuscationOpt.hpp"

// Forward declarations
namespace pybind11 {
class module_;
} // end namespace pybind11

// Forward declarations
namespace llvm {
class Module;
class Function;
class StructType;
} // end namespace llvm

namespace omvll {

struct ObfuscationConfig;

struct YamlConfig {
  std::string PythonPath;
  std::string OMVLLConfig;
};

void initPythonpath();
void initYamlConfig();

class PyConfig {
public:
  static PyConfig &instance();
  ObfuscationConfig *getUserConfig();
  std::string configPath();

  static constexpr auto DefaultFileName = "omvll_config";
  static constexpr auto EnvKey = "OMVLL_CONFIG";
  static constexpr auto PyEnv_Key = "OMVLL_PYTHONPATH";
  static constexpr auto YamlFile = "omvll.yml";

  static inline YamlConfig YConfig;

  PyConfig(const PyConfig &) = delete;
  PyConfig &operator=(const PyConfig &) = delete;

private:
  PyConfig();
  ~PyConfig();

  std::unique_ptr<pybind11::module_> Mod;
  std::unique_ptr<pybind11::module_> CoreMod;
  std::string ModulePath;
};

} // end namespace omvll

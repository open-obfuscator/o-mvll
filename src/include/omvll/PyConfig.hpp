#ifndef OMVLL_PYCONFIG_H
#define OMVLL_PYCONFIG_H
#include <string>
#include <memory>
#include <vector>
#include "omvll/passes/ObfuscationOpt.hpp"

namespace pybind11 {
class module_;
}
namespace llvm {
class Module;
class Function;
class StructType;
}

namespace omvll {
struct ObfuscationConfig;

struct yaml_config_t {
  std::string PYTHONPATH;
  std::string OMVLL_CONFIG;
};

void init_pythonpath();
void init_yamlconfig();


class PyConfig {
  public:

  static PyConfig& instance();
  ObfuscationConfig* getUserConfig();
  const std::vector<std::string>& get_passes();
  std::string config_path();

  static inline const char DEFAULT_FILE_NAME[] = "omvll_config";
  static inline const char ENV_KEY[]           = "OMVLL_CONFIG";
  static inline const char PYENV_KEY[]         = "OMVLL_PYTHONPATH";
  static inline const char YAML_FILE[]         = "omvll.yml";

  inline static yaml_config_t yconfig;
  private:
  PyConfig();
  ~PyConfig();
  static void destroy();

  std::unique_ptr<pybind11::module_> mod_;
  std::unique_ptr<pybind11::module_> core_mod_;
  inline static PyConfig* instance_ = nullptr;
};
}
#endif

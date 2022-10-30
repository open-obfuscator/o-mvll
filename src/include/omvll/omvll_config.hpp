#ifndef OMVLL_CONFIG_ALT_H
#define OMVLL_CONFIG_ALT_H
#include <string>
#include <vector>

namespace omvll {
struct config_t {
  std::vector<std::string> passes;
  bool shuffle_functions;
  bool inline_jni_wrappers;
};

// Defined in omvll_config.cpp
extern config_t config;
void init_default_config();

}

#endif

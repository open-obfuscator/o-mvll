#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <string>
#include <vector>

namespace omvll {

struct OMVLLConfig {
  std::vector<std::string> Passes;
  bool Cleaning;
  bool ShuffleFunctions;
  bool InlineJniWrappers;
};

// Defined in omvll_config.cpp.
extern OMVLLConfig Config;
void initDefaultConfig();

} // end namespace omvll

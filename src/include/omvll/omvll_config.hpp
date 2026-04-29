#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <map>
#include <string>
#include <vector>

namespace omvll {

enum class Phase {
  Early,
  Last,
};

enum class Pass {
  AntiHook,
  StringEncoding,
  OpaqueFieldAccess,
  ControlFlowFlattening,
  BreakControlFlow,
  OpaqueConstants,
  Arithmetic,
  IndirectBranch,
  IndirectCall,
  BasicBlockDuplicate,
  FunctionOutline,
  Cleaning,
};

struct OMVLLConfig {
  std::vector<std::string> Passes;
  std::map<Pass, std::vector<Phase>> PassPhases;
  std::vector<std::string> GlobalModuleExclude;
  std::vector<std::string> GlobalFunctionExclude;
  std::string OutputFolder;
  bool Cleaning;
  bool ShuffleFunctions;
  bool InlineJniWrappers;
  int ProbabilitySeed;
};

// Defined in omvll_config.cpp.
extern OMVLLConfig Config;
void initDefaultConfig();
bool hasPhase(const std::string &PassName, Phase P);

} // end namespace omvll

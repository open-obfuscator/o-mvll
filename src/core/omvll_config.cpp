//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <optional>
#include <unordered_map>

#include "omvll/omvll_config.hpp"

namespace omvll {

OMVLLConfig Config;

void initDefaultConfig() {
  Config.PassPhases.clear();

  Config.Cleaning = true;
  Config.InlineJniWrappers = true;
  Config.ShuffleFunctions = true;
  Config.GlobalModuleExclude.clear();
  Config.GlobalFunctionExclude.clear();
  Config.ProbabilitySeed = 1;
  Config.OutputFolder = "";
}

static std::optional<Pass> nameToPass(const std::string &InternalName) {
  static const std::unordered_map<std::string, Pass> Table = {
      {"omvll::AntiHook",              Pass::AntiHook},
      {"omvll::StringEncoding",        Pass::StringEncoding},
      {"omvll::OpaqueFieldAccess",     Pass::OpaqueFieldAccess},
      {"omvll::ControlFlowFlattening", Pass::ControlFlowFlattening},
      {"omvll::BreakControlFlow",      Pass::BreakControlFlow},
      {"omvll::OpaqueConstants",       Pass::OpaqueConstants},
      {"omvll::Arithmetic",            Pass::Arithmetic},
      {"omvll::IndirectBranch",        Pass::IndirectBranch},
      {"omvll::IndirectCall",          Pass::IndirectCall},
      {"omvll::BasicBlockDuplicate",   Pass::BasicBlockDuplicate},
      {"omvll::FunctionOutline",       Pass::FunctionOutline},
      {"omvll::Cleaning",              Pass::Cleaning},
  };
  auto It = Table.find(InternalName);
  if (It == Table.end())
    return std::nullopt;
  return It->second;
}

bool hasPhase(const std::string &PassName, Phase P) {
  auto MaybePass = nameToPass(PassName);
  if (!MaybePass)
    return P == Phase::Early;
  auto It = Config.PassPhases.find(*MaybePass);
  if (It == Config.PassPhases.end())
    return P == Phase::Early;
  return It->second.count(P) > 0;
}

} // end namespace omvll

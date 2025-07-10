//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "omvll/omvll_config.hpp"
#include "omvll/passes.hpp"

namespace omvll {

OMVLLConfig Config;

void initDefaultConfig() {
  Config.Passes = {
      AntiHook::name().str(),
      StringEncoding::name().str(),

      OpaqueFieldAccess::name().str(),
      ControlFlowFlattening::name().str(),
      BreakControlFlow::name().str(),

      OpaqueConstants::name().str(),
      Arithmetic::name().str(),
      IndirectBranch::name().str(),
      IndirectCall::name().str(),

      // Last pass.
      Cleaning::name().str(),
  };

  Config.Cleaning = true;
  Config.InlineJniWrappers = true;
  Config.ShuffleFunctions = true;
  Config.GlobalModuleExclude.clear();
  Config.GlobalFunctionExclude.clear();
  Config.ProbabilitySeed = 1;
}

} // end namespace omvll

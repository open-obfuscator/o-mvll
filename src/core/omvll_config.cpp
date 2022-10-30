#include "omvll/omvll_config.hpp"
#include "omvll/passes.hpp"

namespace omvll {
config_t config;
void init_default_config() {
  config.passes = {
    AntiHook::name().str(),
    StringEncoding::name().str(),

    OpaqueFieldAccess::name().str(),
    ControlFlowFlattening::name().str(),
    BreakControlFlow::name().str(),

    OpaqueConstants::name().str(),
    Arithmetic::name().str(),

    // Last pass
    Cleaning::name().str(),
  };

  config.inline_jni_wrappers = true;
  config.shuffle_functions   = true;
}
}



#ifndef OMVLL_ANTI_HOOK_OPT_H
#define OMVLL_ANTI_HOOK_OPT_H

namespace omvll {
struct AntiHookOpt {
  AntiHookOpt(bool val) : value(val) {}
  operator bool() const { return value; }
  bool value = false;
};
}

#endif

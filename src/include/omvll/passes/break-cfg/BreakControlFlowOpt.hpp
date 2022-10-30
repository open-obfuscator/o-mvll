#ifndef OMVLL_BREAK_CONTROL_FLOW_OPT_H
#define OMVLL_BREAK_CONTROL_FLOW_OPT_H
#include <variant>
#include <cstddef>
#include <string>

namespace omvll {

struct BreakControlFlowOpt {
  BreakControlFlowOpt(bool val) : value(val) {}
  operator bool() const { return value; }
  bool value = false;
};

}

#endif

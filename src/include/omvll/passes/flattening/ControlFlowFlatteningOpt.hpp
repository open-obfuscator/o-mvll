#ifndef OMVLL_CFG_FLAT_OPT_H
#define OMVLL_CFG_FLAT_OPT_H
#include <variant>

namespace omvll {

struct ControlFlowFlatteningOpt {
  ControlFlowFlatteningOpt(bool val) : value(val) {}
  operator bool() const { return value; }
  bool value = false;
};

}

#endif

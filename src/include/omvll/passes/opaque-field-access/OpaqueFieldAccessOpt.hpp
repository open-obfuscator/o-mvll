#ifndef OMVLL_OPAQUE_FIELD_OPT_H
#define OMVLL_OPAQUE_FIELD_OPT_H
#include <variant>

namespace omvll {

struct StructAccessOpt {
  StructAccessOpt(bool val) : value(val) {}
  operator bool() const { return value; }
  bool value = false;
};

struct VarAccessOpt {
  VarAccessOpt(bool val) : value(val) {}
  operator bool() const { return value; }
  bool value = false;
};

}

#endif

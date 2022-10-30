#ifndef OMVLL_OBJCLEANER_OPT_H
#define OMVLL_OBJCLEANER_OPT_H
#include <variant>

namespace omvll {

struct ObjCleanerOpt {
  ObjCleanerOpt(bool val) : value(val) {}
  operator bool() const { return value; }
  bool value = false;
};

}

#endif

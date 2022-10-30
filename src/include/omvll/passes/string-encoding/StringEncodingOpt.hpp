#ifndef OMVLL_STRING_ENCODING_OPT_H
#define OMVLL_STRING_ENCODING_OPT_H
#include <variant>
#include <cstddef>
#include <string>

namespace omvll {

struct StringEncOptStack {
  size_t loopThreshold = 10;
};

struct StringEncOptSkip{};
struct StringEncOptGlobal{};
struct StringEncOptDefault{};

struct StringEncOptReplace {
  StringEncOptReplace() = default;
  StringEncOptReplace(std::string str) : newString(std::move(str)) {};
  std::string newString;
};


using StringEncodingOpt = std::variant<
    StringEncOptSkip,
    StringEncOptStack,
    StringEncOptGlobal,
    StringEncOptReplace,
    StringEncOptDefault
  >;

}

#endif

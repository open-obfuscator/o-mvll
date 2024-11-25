#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <variant>
#include <cstddef>
#include <string>

namespace omvll {

struct StringEncOptStack {
  size_t LoopThreshold = 10;
};

struct StringEncOptSkip {};
struct StringEncOptGlobal {};
struct StringEncOptDefault {};

struct StringEncOptReplace {
  StringEncOptReplace() = default;
  StringEncOptReplace(std::string Str) : NewString(std::move(Str)) {};
  std::string NewString;
};

using StringEncodingOpt = std::variant<
    StringEncOptSkip,
    StringEncOptStack,
    StringEncOptGlobal,
    StringEncOptReplace,
    StringEncOptDefault
  >;

} // end namespace omvll

#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <variant>
#include <cstddef>
#include <string>

namespace omvll {

struct StringEncOptSkip {};
struct StringEncOptGlobal {};
struct StringEncOptLocal {};
struct StringEncOptDefault {};

struct StringEncOptReplace {
  StringEncOptReplace() = default;
  StringEncOptReplace(std::string Str) : NewString(std::move(Str)) {};
  std::string NewString;
};

using StringEncodingOpt =
    std::variant<StringEncOptSkip, StringEncOptLocal, StringEncOptGlobal,
                 StringEncOptReplace, StringEncOptDefault>;

} // end namespace omvll

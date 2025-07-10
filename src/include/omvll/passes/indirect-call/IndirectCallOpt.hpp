#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <optional>

namespace omvll {

struct IndirectCallConfig {
  IndirectCallConfig(bool Value) : Value(Value) {}
  operator bool() const { return Value; }
  bool Value = false;
};

using IndirectCallOpt = std::optional<IndirectCallConfig>;

} // end namespace omvll

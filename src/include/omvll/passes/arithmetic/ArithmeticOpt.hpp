#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <cstdint>
#include <cstddef>

namespace omvll {

struct ArithmeticOpt {
  static constexpr size_t DefaultNumRounds = 3;
  ArithmeticOpt() = default;
  ArithmeticOpt(uint8_t Iterations) : Iterations(Iterations) {}
  ArithmeticOpt(bool Value) : Iterations(Value ? DefaultNumRounds : 0) {}
  operator bool() const { return Iterations > 0; }
  uint8_t Iterations = DefaultNumRounds;
};

} // end namespace omvll

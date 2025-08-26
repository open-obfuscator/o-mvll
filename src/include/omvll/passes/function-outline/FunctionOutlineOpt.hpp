#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <variant>

namespace omvll {

struct FunctionOutlineSkip {};

struct FunctionOutlineWithProbability {
  FunctionOutlineWithProbability(unsigned Probability = 0)
      : Probability(Probability) {}
  operator bool() const { return Probability > 0; }
  unsigned Probability;
};

using FunctionOutlineOpt =
    std::variant<FunctionOutlineSkip, FunctionOutlineWithProbability>;

} // end namespace omvll

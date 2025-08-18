#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <variant>

namespace omvll {

struct BasicBlockDuplicateSkip {};

struct BasicBlockDuplicateWithProbability {
  BasicBlockDuplicateWithProbability(unsigned Probability = 0)
      : Probability(Probability) {}
  operator bool() const { return Probability > 0; }
  unsigned Probability;
};

using BasicBlockDuplicateOpt =
    std::variant<BasicBlockDuplicateSkip, BasicBlockDuplicateWithProbability>;

} // end namespace omvll

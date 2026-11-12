#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <optional>

namespace omvll {

struct IndirectBranchConfig {
  IndirectBranchConfig(bool Value) : Value(Value) {}
  IndirectBranchConfig(bool Value, unsigned MaxBranches)
      : Value(Value), MaxBranches(MaxBranches) {}
  operator bool() const { return Value; }
  bool Value = false;
  // Skip terminators with more successors than this. 0 means no limit.
  unsigned MaxBranches = 0;
};

using IndirectBranchOpt = std::optional<IndirectBranchConfig>;

} // end namespace omvll

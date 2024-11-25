#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <cstddef>
#include <variant>
#include <vector>

#include "llvm/ADT/DenseSet.h"

namespace omvll {

struct OpaqueConstantsSkip {};

struct OpaqueConstantsBool  {
  OpaqueConstantsBool(bool Value) : Value(Value) {}
  operator bool() const { return Value; }
  bool Value = false;
};

struct OpaqueConstantsLowerLimit {
  OpaqueConstantsLowerLimit(uint64_t Value) : Value(Value) {}
  operator bool() const { return Value > 0; }
  uint64_t Value = 0;
};

struct OpaqueConstantsSet {
  OpaqueConstantsSet(std::vector<uint64_t> Value)
      : Values(Value.begin(), Value.end()) {}

  inline bool contains(uint64_t Value) const { return Values.contains(Value); }
  inline bool empty() { return Values.empty(); };
  inline operator bool() const { return !Values.empty(); }
  llvm::DenseSet<uint64_t> Values;
};

using OpaqueConstantsOpt = std::variant<
  OpaqueConstantsSkip,
  OpaqueConstantsBool,
  OpaqueConstantsLowerLimit,
  OpaqueConstantsSet
>;

} // end namespace omvll

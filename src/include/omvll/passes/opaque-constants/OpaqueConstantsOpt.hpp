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
  OpaqueConstantsBool(bool Value, uint8_t ArithRounds = 0)
      : Value(Value), ArithRounds(ArithRounds) {}
  operator bool() const { return Value; }
  bool Value = false;
  uint8_t ArithRounds = 0;
};

struct OpaqueConstantsLowerLimit {
  OpaqueConstantsLowerLimit(uint64_t Value, uint8_t ArithRounds = 0)
      : Value(Value), ArithRounds(ArithRounds) {}
  operator bool() const { return Value > 0; }
  uint64_t Value = 0;
  uint8_t ArithRounds = 0;
};

struct OpaqueConstantsSet {
  OpaqueConstantsSet(std::vector<uint64_t> Value, uint8_t ArithRounds = 0)
      : Values(Value.begin(), Value.end()), ArithRounds(ArithRounds) {}

  inline bool contains(uint64_t Value) const { return Values.contains(Value); }
  inline bool empty() { return Values.empty(); };
  inline operator bool() const { return !Values.empty(); }
  llvm::DenseSet<uint64_t> Values;
  uint8_t ArithRounds = 0;
};

struct OpaqueConstantsExceptSet {
  OpaqueConstantsExceptSet(std::vector<uint64_t> Value, uint8_t ArithRounds = 0)
      : Values(Value.begin(), Value.end()), ArithRounds(ArithRounds) {}

  inline bool contains(uint64_t Value) const { return Values.contains(Value); }
  inline bool empty() { return Values.empty(); };
  inline operator bool() const { return !Values.empty(); }
  llvm::DenseSet<uint64_t> Values;
  uint8_t ArithRounds = 0;
};

using OpaqueConstantsOpt = std::variant<
  OpaqueConstantsSkip,
  OpaqueConstantsBool,
  OpaqueConstantsLowerLimit,
  OpaqueConstantsSet,
  OpaqueConstantsExceptSet
>;

} // end namespace omvll

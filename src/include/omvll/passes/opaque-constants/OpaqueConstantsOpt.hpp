#ifndef OMVLL_OPAQUE_CST_OPT_H
#define OMVLL_OPAQUE_CST_OPT_H
#include <variant>
#include <cstddef>
#include <string>
#include <vector>

#include <llvm/ADT/DenseSet.h>

namespace omvll {

struct OpaqueConstantsSkip{};

struct OpaqueConstantsBool  {
  OpaqueConstantsBool(bool val) : value(val) {}
  operator bool() const { return value; }
  bool value = false;
};

struct OpaqueConstantsLowerLimit {
  OpaqueConstantsLowerLimit(uint64_t val) : value(val) {}
  operator bool() const { return value > 0; }
  uint64_t value = 0;
};

struct OpaqueConstantsSet {
  OpaqueConstantsSet(std::vector<uint64_t> val) :
    values(val.begin(), val.end())
  {}
  inline bool contains(uint64_t value) const { return values.contains(value); }
  inline bool empty() { return values.empty(); };
  inline operator bool() const { return !values.empty(); }
  llvm::DenseSet<uint64_t> values;
};

using OpaqueConstantsOpt = std::variant<
  OpaqueConstantsSkip,
  OpaqueConstantsBool,
  OpaqueConstantsLowerLimit,
  OpaqueConstantsSet
>;




}

#endif

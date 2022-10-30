#ifndef OMVLL_ARITHMETIC_OPT_H
#define OMVLL_ARITHMETIC_OPT_H
#include <cstdint>
#include <cstddef>

namespace omvll {

struct ArithmeticOpt {
  static constexpr size_t DEFAULT_NB_ROUND = 3;
  ArithmeticOpt() = default;
  ArithmeticOpt(uint8_t it) : iterations(it) {}
  ArithmeticOpt(bool val) : iterations(val ? DEFAULT_NB_ROUND : 0) {}
  operator bool() const { return iterations > 0; }
  uint8_t iterations = DEFAULT_NB_ROUND;
};


}

#endif

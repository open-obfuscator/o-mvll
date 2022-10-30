#ifndef OMVLL_VARIANT_VISIT_H
#define OMVLL_VARIANT_VISIT_H
#include <variant>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
#endif

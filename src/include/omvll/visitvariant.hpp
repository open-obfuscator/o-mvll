#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <variant>

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

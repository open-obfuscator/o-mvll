#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>

namespace omvll {

pybind11::module_ &py_init_log(pybind11::module_ &M);
pybind11::module_ &py_init_llvm_bindings(pybind11::module_ &M);
pybind11::module_ &py_init_obf_opt(pybind11::module_ &M);

} // end namespace omvll

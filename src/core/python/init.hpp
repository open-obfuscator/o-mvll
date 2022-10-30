#ifndef OMVLL_PY_INIT_H
#define OMVLL_PY_INIT_H
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace omvll {
py::module_& py_init_log(py::module_& m);
py::module_& py_init_llvm_bindings(py::module_& m);
py::module_& py_init_obf_opt(py::module_& m);

}
#endif

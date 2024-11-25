//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "omvll/log.hpp"

#include "init.hpp"

namespace py = pybind11;

namespace omvll {

py::module_ &py_init_log(py::module_ &m) {
  py::enum_<LogLevel>(m, "LogLevel")
      .value("DEBUG", LogLevel::Debug)
      .value("TRACE", LogLevel::Trace)
      .value("INFO", LogLevel::Info)
      .value("WARN", LogLevel::Warn)
      .value("ERR", LogLevel::Err);

  m.def("set_log_level", py::overload_cast<LogLevel>(&Logger::set_level));
  return m;
}

} // end namespace omvll

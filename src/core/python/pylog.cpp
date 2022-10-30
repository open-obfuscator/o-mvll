#include "omvll/log.hpp"
#include "init.hpp"

namespace omvll {

py::module_& py_init_log(py::module_& m) {

  py::enum_<LOG_LEVEL>(m, "LOG_LEVEL")
    .value("DEBUG", LOG_LEVEL::DEBUG)
    .value("TRACE", LOG_LEVEL::TRACE)
    .value("INFO", LOG_LEVEL::INFO)
    .value("WARN", LOG_LEVEL::WARN)
    .value("ERR", LOG_LEVEL::ERR);

  m.def("set_log_level", py::overload_cast<LOG_LEVEL>(&Logger::set_level));
  return m;
}

}

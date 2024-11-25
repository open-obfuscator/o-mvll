//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <dlfcn.h>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include "omvll/PyConfig.hpp"
#include "omvll/omvll_config.hpp"
#include "omvll/utils.hpp"
#include "omvll/versioning.hpp"

#include "PyObfuscationConfig.hpp"
#include "init.hpp"

namespace py = pybind11;

using namespace pybind11::literals;

namespace omvll {

void initPythonpath() {
  if (char *Config = getenv(PyConfig::PyEnv_Key)) {
    Py_SetPath(Py_DecodeLocale(Config, nullptr));
    setenv("PYTHONHOME", Config, true);
    return;
  }

  if (!PyConfig::YConfig.PythonPath.empty()) {
    Py_SetPath(Py_DecodeLocale(PyConfig::YConfig.PythonPath.c_str(), nullptr));
    setenv("PYTHONHOME", PyConfig::YConfig.PythonPath.c_str(), true);
    return;
  }

#if defined(__linux__)
  if (auto *Hdl = dlopen("libpython3.10.so", RTLD_LAZY)) {
    char Path[400];
    int Ret = dlinfo(Hdl, RTLD_DI_ORIGIN, Path);
    if (Ret != 0)
      return;

    std::string PythonPath = Path;
    PythonPath.append("/python3.10");
    Py_SetPath(Py_DecodeLocale(PythonPath.c_str(), nullptr));
    setenv("PYTHONHOME", PythonPath.c_str(), true);
    return;
  }
#endif
}

void OMVLLCtor(py::module_ &m) {
  initDefaultConfig();

  m.attr("LLVM_VERSION")  = OMVLL_LLVM_VERSION_STRING;
  m.attr("OMVLL_VERSION") = OMVLL_VERSION;
  m.attr("OMVLL_VERSION_FULL") = "OMVLL Version: " OMVLL_VERSION " / " OMVLL_LLVM_VERSION_STRING
                                 " (" OMVLL_LLVM_VERSION ")";

  py::class_<OMVLLConfig>(m, "OMVLLConfig",
                          R"delim(
    This class is used to configure the global behavior of O-MVLL.

    It can be accessed through the global :attr:`omvll.config` attribute
    )delim")
      .def_readwrite("passes", &OMVLLConfig::Passes,
                     R"delim(
                   This **ordered** list contains the sequence of the obfuscation passes
                   that must be used.
                   It should not be modified unless you know what you do.

                   This attribute is set by default to these values:

                   |omvll-passes|

                   )delim")

      .def_readwrite("inline_jni_wrappers", &OMVLLConfig::InlineJniWrappers,
                     R"delim(
                   This boolean attribute is used to force inlining JNI C++ wrapper.
                   For instance ``GetStringChars``:

                   .. code-block:: cpp

                     const jchar* GetStringChars(jstring string, jboolean* isCopy)
                     { return functions->GetStringChars(this, string, isCopy); }

                   The default value is ``True``.
                   )delim")

      .def_readwrite("shuffle_functions", &OMVLLConfig::ShuffleFunctions,
                     R"delim(
                    Whether the postition of Module's functions should be shuffled.

                    This randomization is used to avoid the same (relative) position of the functions
                    for two different builds of the protected binary.

                    For instance, if the original source code is composed of the following functions:

                    .. code-block:: cpp

                        void hello();
                        void hello_world();
                        void say_hi();

                    In the final (stripped) binary, the functions appear as the following sequence:

                    .. code-block:: text

                        sub_3455() // hello
                        sub_8A74() // hello_world
                        sub_AF34() // say_hi

                    If this value is set to ``True`` (which is the default value), the sequence is randomized.
                    )delim");

  m.attr("config") = &Config;

  py_init_obf_opt(m);
  py_init_llvm_bindings(m);
  py_init_log(m);

  py::class_<ObfuscationConfig, PyObfuscationConfig>(m, "ObfuscationConfig",
                                                     R"delim(
    This class must be inherited by the user to define where and how the obfuscation
    passes must be enabled.
    )delim")
      .def(py::init<>())

      .def("obfuscate_string", &ObfuscationConfig::obfuscateString,
           R"delim(
         The default user-callback used to configure strings obfuscation.

         In addition to the associated class options, O-MVLL interprets these return values as follows:

         +--------------+-------------------------------------+
         | Return Value | Interpretation                      |
         +==============+=====================================+
         | ``None``     | :class:`~omvll.StringEncOptSkip`    |
         +--------------+-------------------------------------+
         | ``False``    | :class:`~omvll.StringEncOptSkip`    |
         +--------------+-------------------------------------+
         | ``True``     | :class:`~omvll.StringEncOptDefault` |
         +--------------+-------------------------------------+
         | ``str``      | :class:`~omvll.StringEncOptReplace` |
         +--------------+-------------------------------------+
         | ``bytes``    | :class:`~omvll.StringEncOptReplace` |
         +--------------+-------------------------------------+

         See the :omvll:`strings-encoding` documentation.
         )delim",
           "module"_a, "function"_a, "string"_a)

      .def("break_control_flow", &ObfuscationConfig::breakControlFlow,
           R"delim(
         The default user-callback for the pass that breaks
         the control flow.

         In addition to the associated class options, O-MVLL interprets these return values as follows:

         +--------------+-------------------------------------------------+
         | Return Value | Interpretation                                  |
         +==============+=================================================+
         | ``True``     | :class:`~omvll.BreakControlFlowOpt`\(``True``)  |
         +--------------+-------------------------------------------------+
         | ``False``    | :class:`~omvll.BreakControlFlowOpt`\(``False``) |
         +--------------+-------------------------------------------------+
         | ``None``     | :class:`~omvll.BreakControlFlowOpt`\(``False``) |
         +--------------+-------------------------------------------------+

         See the :omvll:`control-flow-breaking` documentation.
         )delim",
           "module"_a, "function"_a)

      .def("flatten_cfg", &ObfuscationConfig::controlFlowGraphFlattening,
           R"delim(
         The default user-callback used to configure the
         control-flow flattening pass.

         In addition to the associated class options, O-MVLL interprets these return values as follows:

         +--------------+------------------------------------------------------+
         | Return Value | Interpretation                                       |
         +==============+======================================================+
         | ``True``     | :class:`~omvll.ControlFlowFlatteningOpt`\(``True``)  |
         +--------------+------------------------------------------------------+
         | ``False``    | :class:`~omvll.ControlFlowFlatteningOpt`\(``False``) |
         +--------------+------------------------------------------------------+
         | ``None``     | :class:`~omvll.ControlFlowFlatteningOpt`\(``False``) |
         +--------------+------------------------------------------------------+

         See the :omvll:`control-flow-flattening` documentation.
         )delim",
           "module"_a, "function"_a)

      .def("obfuscate_struct_access", &ObfuscationConfig::obfuscateStructAccess,
           R"delim(
         The default user-callback when obfuscating structures accesses.

         In addition to the associated class options, O-MVLL interprets these return values as follows:

         +--------------+---------------------------------------------+
         | Return Value | Interpretation                              |
         +==============+=============================================+
         | ``True``     | :class:`~omvll.StructAccessOpt`\(``True``)  |
         +--------------+---------------------------------------------+
         | ``False``    | :class:`~omvll.StructAccessOpt`\(``False``) |
         +--------------+---------------------------------------------+
         | ``None``     | :class:`~omvll.StructAccessOpt`\(``False``) |
         +--------------+---------------------------------------------+

         See the :omvll:`opaque-fields-access` documentation.
         )delim",
           "module"_a, "function"_a, "struct"_a)

      .def("obfuscate_variable_access",
           &ObfuscationConfig::obfuscateVariableAccess,
           R"delim(
         The default user-callback when obfuscating global variables access.

         In addition to the associated class options, O-MVLL interprets these return values as follows:

         +--------------+------------------------------------------+
         | Return Value | Interpretation                           |
         +==============+==========================================+
         | ``True``     | :class:`~omvll.VarAccessOpt`\(``True``)  |
         +--------------+------------------------------------------+
         | ``False``    | :class:`~omvll.VarAccessOpt`\(``False``) |
         +--------------+------------------------------------------+
         | ``None``     | :class:`~omvll.VarAccessOpt`\(``False``) |
         +--------------+------------------------------------------+

         See the :omvll:`opaque-fields-access` documentation.
         )delim",
           "module"_a, "function"_a, "variable"_a)

      .def("obfuscate_constants", &ObfuscationConfig::obfuscateConstants,
           R"delim(
         The default user-callback to obfuscate constants.

         In addition to the associated class options, O-MVLL interprets these return values as follows:

         +-------------------+--------------------------------------------------------+
         | Return Value      | Interpretation                                         |
         +===================+========================================================+
         | ``True``          | :class:`~omvll.OpaqueConstantsBool`\(``True``)         |
         +-------------------+--------------------------------------------------------+
         | ``False``         | :class:`~omvll.OpaqueConstantsBool`\(``False``)        |
         +-------------------+--------------------------------------------------------+
         | ``None``          | :class:`~omvll.OpaqueConstantsBool`\(``False``)        |
         +-------------------+--------------------------------------------------------+
         | ``list(int ...)`` | :class:`~omvll.omvll.OpaqueConstantsSet`\(``int ...``) |
         +-------------------+--------------------------------------------------------+

         See the :omvll:`opaque-constants` documentation.
         )delim",
           "module"_a, "function"_a)

      .def("obfuscate_arithmetic", &ObfuscationConfig::obfuscateArithmetics,
           R"delim(
         The default user-callback when obfuscating arithmetic operations.

         In addition to the associated class options, O-MVLL interprets these return values as follows:

         +--------------+-------------------------------------------+
         | Return Value | Interpretation                            |
         +==============+===========================================+
         | ``True``     | :class:`~omvll.ArithmeticOpt`\(``True``)  |
         +--------------+-------------------------------------------+
         | ``False``    | :class:`~omvll.ArithmeticOpt`\(``False``) |
         +--------------+-------------------------------------------+
         | ``None``     | :class:`~omvll.ArithmeticOpt`\(``False``) |
         +--------------+-------------------------------------------+

         See the :omvll:`arithmetic` documentation.
         )delim",
           "module"_a, "function"_a)

      .def("anti_hooking", &ObfuscationConfig::antiHooking,
           R"delim(
         The default user-callback to enable hooking protection.

         In addition to the associated class options, O-MVLL interprets these return values as follows:

         +--------------+-----------------------------------------+
         | Return Value | Interpretation                          |
         +==============+=========================================+
         | ``True``     | :class:`~omvll.AntiHookOpt`\(``True``)  |
         +--------------+-----------------------------------------+
         | ``False``    | :class:`~omvll.AntiHookOpt`\(``False``) |
         +--------------+-----------------------------------------+
         | ``None``     | :class:`~omvll.AntiHookOpt`\(``False``) |
         +--------------+-----------------------------------------+

         See the :omvll:`anti-hook` documentation.
         )delim",
           "module"_a, "function"_a)

      .def("report_diff", &ObfuscationConfig::reportDiff,
           R"delim(
         User-callback to monitor IR-level changes from individual obfuscation passes.
         )delim",
           "pass_name"_a, "original"_a, "obfuscated"_a);
}

std::unique_ptr<py::module_> initOMVLLCore(py::dict Modules) {
  auto M = std::make_unique<py::module_>(
      py::module_::create_extension_module("omvll", "", new PyModuleDef()));
  OMVLLCtor(*M);
  Modules["omvll"] = *M;
  return M;
}

PyConfig::~PyConfig() = default;

PyConfig &PyConfig::instance() {
  if (!Instance) {
    Instance = new PyConfig{};
    std::atexit(destroy);
  }
  return *Instance;
}

ObfuscationConfig *PyConfig::getUserConfig() {
  try {
    llvm::LLVMContext Ctx;
    if (!py::hasattr(*Mod, "omvll_get_config"))
      fatalError("Missing omvll_get_config");

    auto PyUserConfig = Mod->attr("omvll_get_config");
    if (PyUserConfig.is_none())
      fatalError("Missing omvll_get_config");

    py::object Result = PyUserConfig();
    return Result.cast<ObfuscationConfig *>();
  } catch (const std::exception &Exc) {
    fatalError(Exc.what());
  }
}

PyConfig::PyConfig() {
  py::initialize_interpreter();
  py::module_ SysMod = py::module_::import("sys");
  py::module_ PathLib = py::module_::import("pathlib");
  py::dict Modules = SysMod.attr("modules");

  CoreMod = initOMVLLCore(Modules);

  llvm::StringRef ConfigPath;
  if (char *Config = getenv(EnvKey))
    ConfigPath = Config;
  else if (!PyConfig::YConfig.OMVLLConfig.empty())
    ConfigPath = PyConfig::YConfig.OMVLLConfig;

  std::string ModName = DefaultFileName;
  if (!ConfigPath.empty()) {
    std::string Config = ConfigPath.str();
    auto PyPath = PathLib.attr("Path")(Config);
    py::list Path = SysMod.attr("path");
    Path.insert(0, PyPath.attr("parent").attr("as_posix")());
    std::string Name = PyPath.attr("stem").cast<std::string>();
    ModName = Name;
  }

  try {
    Mod = std::make_unique<py::module_>(py::module_::import(ModName.c_str()));
  } catch (const std::exception &Exc) {
    fatalError(Exc.what());
  }
}

std::string PyConfig::configPath() {
  return Mod->attr("__file__").cast<std::string>();
}

void PyConfig::destroy() {
  delete Instance;
  py::finalize_interpreter();
}

} // end namespace omvll

PYBIND11_MODULE(omvll, m) { omvll::OMVLLCtor(m); }

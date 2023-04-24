#include "omvll/PyConfig.hpp"
#include "omvll/passes.hpp"
#include "omvll/utils.hpp"
#include "omvll/log.hpp"
#include "omvll/versioning.hpp"
#include "omvll/omvll_config.hpp"

#include "PyObfuscationConfig.hpp"
#include "init.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>

#include <dlfcn.h>

#include <llvm/Support/Compiler.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>

namespace py = pybind11;

namespace omvll {


void init_pythonpath() {
  if (char* config = getenv(PyConfig::PYENV_KEY)) {
    Py_SetPath(Py_DecodeLocale(config, nullptr));
    setenv("PYTHONHOME", config, true);
    return;
  }

  if (!PyConfig::yconfig.PYTHONPATH.empty()) {
    Py_SetPath(Py_DecodeLocale(PyConfig::yconfig.PYTHONPATH.c_str(), nullptr));
    setenv("PYTHONHOME", PyConfig::yconfig.PYTHONPATH.c_str(), true);
    return;
  }
  #if defined(__linux__)
    if (auto* hdl = dlopen("libpython3.10.so", RTLD_LAZY)) {
      char PATH[400];
      int ret = dlinfo(hdl, RTLD_DI_ORIGIN, PATH);
      if (ret != 0) {
        return;
      }
      std::string PythonPath = PATH;
      PythonPath.append("/python3.10");
      Py_SetPath(Py_DecodeLocale(PythonPath.c_str(), nullptr));
      setenv("PYTHONHOME", PythonPath.c_str(), true);
      return;
    }
  #endif
}

void omvll_ctor(py::module_& m) {
  init_default_config();

  m.attr("LLVM_VERSION")  = OMVLL_LLVM_VERSION_STRING;
  m.attr("OMVLL_VERSION") = OMVLL_VERSION;
  m.attr("OMVLL_VERSION_FULL") = "OMVLL Version: " OMVLL_VERSION " / " OMVLL_LLVM_VERSION_STRING
                                 " (" OMVLL_LLVM_VERSION ")";

  py::class_<config_t>(m, "config_t",
    R"delim(
    This class is used to configure the global behavior of O-MVLL.

    It can be accessed through the global :attr:`omvll.config` attribute
    )delim")
    .def_readwrite("passes",
                   &config_t::passes,
                   R"delim(
                   This **ordered** list contains the sequence of the obfuscation passes
                   that must be used.
                   It should not be modified unless you know what you do.

                   This attribute is set by default to these values:

                   |omvll-passes|

                   )delim")

    .def_readwrite("inline_jni_wrappers",
                   &config_t::inline_jni_wrappers,
                   R"delim(
                   This boolean attribute is used to force inlining JNI C++ wrapper.
                   For instance ``GetStringChars``:

                   .. code-block:: cpp

                     const jchar* GetStringChars(jstring string, jboolean* isCopy)
                     { return functions->GetStringChars(this, string, isCopy); }

                   The default value is ``True``.
                   )delim")

    .def_readwrite("shuffle_functions",
                    &config_t::shuffle_functions,
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

  m.attr("config") = &config;

  py_init_obf_opt(m);
  py_init_llvm_bindings(m);
  py_init_log(m);

  py::class_<ObfuscationConfig, PyObfuscationConfig>(m, "ObfuscationConfig",
    R"delim(
    This class must be inherited by the user to define where and how the obfuscation
    passes must be enabled.
    )delim")
    .def(py::init<>())

    .def("obfuscate_string",
         &ObfuscationConfig::obfuscate_string,
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
         )delim", "module"_a, "function"_a, "string"_a)

    .def("break_control_flow",
         &ObfuscationConfig::break_control_flow,
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
         )delim", "module"_a, "function"_a)

    .def("flatten_cfg",
         &ObfuscationConfig::flatten_cfg,
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
         )delim", "module"_a, "function"_a)

    .def("obfuscate_struct_access",
         &ObfuscationConfig::obfuscate_struct_access,
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
         )delim", "module"_a, "function"_a, "struct"_a)

    .def("obfuscate_variable_access",
         &ObfuscationConfig::obfuscate_variable_access,
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
         )delim", "module"_a, "function"_a, "variable"_a)

    .def("obfuscate_constants",
         &ObfuscationConfig::obfuscate_constants,
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
         )delim", "module"_a, "function"_a)

    .def("obfuscate_arithmetic",
         &ObfuscationConfig::obfuscate_arithmetic,
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
         )delim", "module"_a, "function"_a)

    .def("anti_hooking",
         &ObfuscationConfig::anti_hooking,
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
         )delim", "module"_a, "function"_a);

}

std::unique_ptr<py::module_> init_omvll_core(py::dict modules) {
  auto m = std::make_unique<py::module_>(py::module_::create_extension_module("omvll", "", new PyModuleDef()));
  omvll_ctor(*m);
  modules["omvll"] = *m;
  return m;
}


PyConfig::~PyConfig() = default;

PyConfig& PyConfig::instance() {
  if (instance_ == nullptr) {
    instance_ = new PyConfig{};
    std::atexit(destroy);
  }
  return *instance_;
}

ObfuscationConfig* PyConfig::getUserConfig() {
  try {
    llvm::LLVMContext Ctx;
    if (!py::hasattr(*mod_, "omvll_get_config")) {
      fatalError("Missing omvll_get_config");
    }
    auto py_user_config = mod_->attr("omvll_get_config");
    if (py_user_config.is_none()) {
      fatalError("Missing omvll_get_config");
    }
    py::object result = py_user_config();
    auto* conf = result.cast<ObfuscationConfig*>();
    return conf;
  } catch (const std::exception& e) {
    fatalError(e.what());
  }
}


const std::vector<std::string>& PyConfig::get_passes() {
  return config.passes;
}

PyConfig::PyConfig() {
  py::initialize_interpreter();
  py::module_ sys_mod = py::module_::import("sys");
  py::module_ pathlib = py::module_::import("pathlib");

  py::dict modules = sys_mod.attr("modules");
  core_mod_ = init_omvll_core(modules);

  llvm::StringRef configpath;

  if (char* config = getenv(ENV_KEY)) {
    configpath = config;
  } else if (!PyConfig::yconfig.OMVLL_CONFIG.empty()) {
    configpath = PyConfig::yconfig.OMVLL_CONFIG;
  }
  std::string modname = DEFAULT_FILE_NAME;
  if (!configpath.empty()) {
    std::string config = configpath.str();

    auto pypath = pathlib.attr("Path")(config);
    py::list path = sys_mod.attr("path");
    path.insert(0, pypath.attr("parent").attr("as_posix")());
    std::string name = pypath.attr("stem").cast<std::string>();
    modname = name;
  }

  try {
    mod_ = std::make_unique<py::module_>(py::module_::import(modname.c_str()));
  } catch (const std::exception& e) {
    fatalError(e.what());
  }
}

std::string PyConfig::config_path() {
  return mod_->attr("__file__").cast<std::string>();
}

void PyConfig::destroy() {
  delete instance_;
  py::finalize_interpreter();
}

}

PYBIND11_MODULE(omvll, m) {
  omvll::omvll_ctor(m);
}

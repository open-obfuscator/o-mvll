#include "PyObfuscationConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/tak-injection/TakInjectionOpt.hpp"
#include "omvll/utils.hpp"

#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

namespace py = pybind11;
using namespace std::string_literals;

namespace omvll {

StringEncodingOpt PyObfuscationConfig::obfuscate_string(llvm::Module* mod, llvm::Function* func,
                                                        const std::string& str)
{
  py::gil_scoped_acquire gil;
  py::function override = py::get_override(static_cast<const ObfuscationConfig*>(this), "obfuscate_string");
  if (override) {
    try {
      py::bytes bytes_str(str);
      py::object out = override(mod, func, bytes_str);
      if (out.is_none()) {
        return StringEncOptSkip();
      }
      if (py::isinstance<py::bool_>(out)) {
        bool val = out.cast<py::bool_>();
        if (!val) {
          return StringEncOptSkip();
        } else {
          return StringEncOptDefault();
        }
      }

      if (py::isinstance<py::str>(out)) {
        auto val = out.cast<std::string>();
        return StringEncOptReplace(std::move(val));
      }

      if (py::isinstance<py::bytes>(out)) {
        auto val = out.cast<std::string>();
        return StringEncOptReplace(std::move(val));
      }

      if (py::detail::cast_is_temporary_value_reference<StringEncodingOpt>::value) {
        static pybind11::detail::override_caster_t<StringEncodingOpt> caster;
        return pybind11::detail::cast_ref<StringEncodingOpt>(std::move(out), caster);
      }
      return pybind11::detail::cast_safe<StringEncodingOpt>(std::move(out));
    } catch (const std::exception& e) {
      fatalError("Error in 'obfuscate_string': '"s + e.what() + "'");
    }
  }

  return StringEncOptSkip();
}

BreakControlFlowOpt PyObfuscationConfig::break_control_flow(llvm::Module* mod, llvm::Function* func) {
  py::gil_scoped_acquire gil;
  py::function override = py::get_override(static_cast<const ObfuscationConfig*>(this), "break_control_flow");
  if (override) {
    try {
      py::object out = override(mod, func);
      if (out.is_none()) {
        return false;
      }
      if (py::isinstance<py::bool_>(out)) {
        bool val = out.cast<py::bool_>();
        return val;
      }
      if (py::detail::cast_is_temporary_value_reference<BreakControlFlowOpt>::value) {
        static pybind11::detail::override_caster_t<BreakControlFlowOpt> caster;
        return pybind11::detail::cast_ref<BreakControlFlowOpt>(std::move(out), caster);
      }
      return pybind11::detail::cast_safe<BreakControlFlowOpt>(std::move(out));
    } catch (const std::exception& e) {
      fatalError("Error in 'break_control_flow': '"s + e.what() + "'");
    }
  }
  return false;
}

ControlFlowFlatteningOpt PyObfuscationConfig::flatten_cfg(llvm::Module* mod, llvm::Function* func) {
  py::gil_scoped_acquire gil;
  py::function override = py::get_override(static_cast<const ObfuscationConfig*>(this), "flatten_cfg");
  if (override) {
    try {
      py::object out = override(mod, func);
      if (out.is_none()) {
        return false;
      }
      if (py::isinstance<py::bool_>(out)) {
        bool val = out.cast<py::bool_>();
        return val;
      }
      if (py::detail::cast_is_temporary_value_reference<ControlFlowFlatteningOpt>::value) {
        static pybind11::detail::override_caster_t<ControlFlowFlatteningOpt> caster;
        return pybind11::detail::cast_ref<ControlFlowFlatteningOpt>(std::move(out), caster);
      }
      return pybind11::detail::cast_safe<ControlFlowFlatteningOpt>(std::move(out));
    } catch (const std::exception& e) {
      fatalError("Error in 'flatten_cfg': '"s + e.what() + "'");
    }
  }
  return false;
}

StructAccessOpt PyObfuscationConfig::obfuscate_struct_access(llvm::Module* mod, llvm::Function* func,
                                                             llvm::StructType* S)
{
  py::gil_scoped_acquire gil;
  py::function override = py::get_override(static_cast<const ObfuscationConfig*>(this), "obfuscate_struct_access");
  if (override) {
    try {
      py::object out = override(mod, func, S);

      if (out.is_none()) {
        return false;
      }

      if (py::isinstance<py::bool_>(out)) {
        bool val = out.cast<py::bool_>();
        return val;
      }

      if (py::detail::cast_is_temporary_value_reference<StringEncodingOpt>::value) {
        static pybind11::detail::override_caster_t<StructAccessOpt> caster;
        return pybind11::detail::cast_ref<StructAccessOpt>(std::move(out), caster);
      }
      return pybind11::detail::cast_safe<StructAccessOpt>(std::move(out));
    } catch (const std::exception& e) {
      fatalError("Error in 'obfuscate_struct_access': '"s + e.what() + "'");
    }
  }
  return false;
}

VarAccessOpt PyObfuscationConfig::obfuscate_variable_access(llvm::Module* mod, llvm::Function* func,
                                                            llvm::GlobalVariable* GV)
{
  py::gil_scoped_acquire gil;
  py::function override = py::get_override(static_cast<const ObfuscationConfig*>(this), "obfuscate_variable_access");
  if (override) {
    try {
      py::object out = override(mod, func, GV);

      if (out.is_none()) {
        return false;
      }

      if (py::isinstance<py::bool_>(out)) {
        bool val = out.cast<py::bool_>();
        return val;
      }

      if (py::detail::cast_is_temporary_value_reference<VarAccessOpt>::value) {
        static pybind11::detail::override_caster_t<VarAccessOpt> caster;
        return pybind11::detail::cast_ref<VarAccessOpt>(std::move(out), caster);
      }
      return pybind11::detail::cast_safe<VarAccessOpt>(std::move(out));
    } catch (const std::exception& e) {
      fatalError("Error in 'obfuscate_variable_access': '"s + e.what() + "'");
    }
  }
  return false;
}

AntiHookOpt PyObfuscationConfig::anti_hooking(llvm::Module* mod, llvm::Function* func)
{
  py::gil_scoped_acquire gil;
  py::function override = py::get_override(static_cast<const ObfuscationConfig*>(this), "anti_hooking");
  if (override) {
    try {
      py::object out = override(mod, func);
      if (out.is_none()) {
        return false;
      }

      if (py::isinstance<py::bool_>(out)) {
        bool res = out.cast<py::bool_>();
        return res;
      }
      if (py::detail::cast_is_temporary_value_reference<AntiHookOpt>::value) {
        static pybind11::detail::override_caster_t<AntiHookOpt> caster;
        return pybind11::detail::cast_ref<AntiHookOpt>(std::move(out), caster);
      }
      return pybind11::detail::cast_safe<AntiHookOpt>(std::move(out));
    } catch (const std::exception& e) {
      fatalError("Error in 'anti_hooking': '"s + e.what() + "'");
    }
  }
  return false;
}

ArithmeticOpt PyObfuscationConfig::obfuscate_arithmetic(llvm::Module* mod, llvm::Function* func)
{
  py::gil_scoped_acquire gil;
  py::function override = py::get_override(static_cast<const ObfuscationConfig*>(this), "obfuscate_arithmetic");
  if (override) {
    try {
      py::object out = override(mod, func);
      if (out.is_none()) {
        return false;
      }

      if (py::isinstance<py::bool_>(out)) {
        bool res = out.cast<py::bool_>();
        return res;
      }
      if (py::detail::cast_is_temporary_value_reference<ArithmeticOpt>::value) {
        static pybind11::detail::override_caster_t<ArithmeticOpt> caster;
        return pybind11::detail::cast_ref<ArithmeticOpt>(std::move(out), caster);
      }
      return pybind11::detail::cast_safe<ArithmeticOpt>(std::move(out));
    } catch (const std::exception& e) {
      fatalError("Error in 'obfuscate_arithmetic': '"s + e.what() + "'");
    }
  }
  return false;
}

OpaqueConstantsOpt PyObfuscationConfig::obfuscate_constants(llvm::Module* mod, llvm::Function* func)
{
  py::gil_scoped_acquire gil;
  py::function override = py::get_override(static_cast<const ObfuscationConfig*>(this), "obfuscate_constants");
  if (override) {
    try {
      py::object out = override(mod, func);
      if (out.is_none()) {
        return OpaqueConstantsSkip();
      }

      if (py::isinstance<py::bool_>(out)) {
        bool res = out.cast<py::bool_>();
        return OpaqueConstantsBool(res);
      }

      if (py::isinstance<py::list>(out)) {
        py::list list = out.cast<py::list>();
        std::vector<uint64_t> values;
        values.reserve(list.size());
        for (size_t i = 0; i < list.size(); ++i) {
          values.push_back(list[i].cast<uint64_t>());
        }
        return OpaqueConstantsSet(std::move(values));
      }

      if (py::detail::cast_is_temporary_value_reference<OpaqueConstantsOpt>::value) {
        static pybind11::detail::override_caster_t<OpaqueConstantsOpt> caster;
        return pybind11::detail::cast_ref<OpaqueConstantsOpt>(std::move(out), caster);
      }
      return pybind11::detail::cast_safe<OpaqueConstantsOpt>(std::move(out));
    } catch (const std::exception& e) {
      fatalError("Error in 'obfuscate_constants': '"s + e.what() + "'");
    }
  }
  return OpaqueConstantsSkip();
}

TakInjectionOpt PyObfuscationConfig::inject_tak(llvm::Module *mod) {
  py::gil_scoped_acquire gil;
  py::function override = py::get_override(
      static_cast<const ObfuscationConfig *>(this), "inject_tak");
  if (override) {
    try {
      py::object out = override(mod);
      if (out.is_none()) {
        return TakInjectionSkip();
      }

      if (py::isinstance<py::tuple>(out)) {
        auto tuple_out = out.cast<py::tuple>();
        if (tuple_out.size() == 3 && py::isinstance<py::str>(tuple_out[0]) &&
            py::isinstance<py::int_>(tuple_out[1]) &&
            py::isinstance<py::list>(tuple_out[2])) {

          auto configPath = tuple_out[0].cast<std::string>();
          auto intervalTime = tuple_out[1].cast<unsigned>();
          auto excludedTargetsPyList = tuple_out[2].cast<py::list>();

          std::vector<std::string> excludedTargets;
          excludedTargets.reserve(excludedTargetsPyList.size());
          for (size_t i = 0; i < excludedTargetsPyList.size(); ++i)
            if (py::isinstance<py::str>(excludedTargetsPyList[i]))
              excludedTargets.push_back(
                  excludedTargetsPyList[i].cast<std::string>());

          return TakInjectionConfig(std::move(configPath), intervalTime,
                                    std::move(excludedTargets));
        }
      }

      if (py::detail::cast_is_temporary_value_reference<
              TakInjectionOpt>::value) {
        static pybind11::detail::override_caster_t<TakInjectionOpt> caster;
        return pybind11::detail::cast_ref<TakInjectionOpt>(std::move(out),
                                                           caster);
      }
      return pybind11::detail::cast_safe<TakInjectionOpt>(std::move(out));
    } catch (const std::exception &e) {
      fatalError("Error in 'inject_tak': '"s + e.what() + "'");
    }
  }
  return TakInjectionSkip();
}
}



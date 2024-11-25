//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

#include "omvll/utils.hpp"

#include "PyObfuscationConfig.hpp"

namespace py = pybind11;
namespace detail = py::detail;

using namespace std::string_literals;

namespace omvll {

StringEncodingOpt PyObfuscationConfig::obfuscateString(llvm::Module *M,
                                                       llvm::Function *F,
                                                       const std::string &Str) {
  py::gil_scoped_acquire gil;
  const auto *Base = static_cast<const ObfuscationConfig *>(this);
  py::function override = py::get_override(Base, "obfuscate_string");
  if (override) {
    try {
      py::bytes bytes_str(Str);
      py::object out = override(M, F, bytes_str);
      if (out.is_none())
        return StringEncOptSkip();

      if (py::isinstance<py::bool_>(out)) {
        bool Value = out.cast<py::bool_>();
        if (!Value)
          return StringEncOptSkip();
        else
          return StringEncOptDefault();
      }

      if (py::isinstance<py::str>(out)) {
        auto Value = out.cast<std::string>();
        return StringEncOptReplace(std::move(Value));
      }

      if (py::isinstance<py::bytes>(out)) {
        auto Value = out.cast<std::string>();
        return StringEncOptReplace(std::move(Value));
      }

      bool is_temp =
          detail::cast_is_temporary_value_reference<StringEncodingOpt>::value;
      if (is_temp) {
        static detail::override_caster_t<StringEncodingOpt> caster;
        return detail::cast_ref<StringEncodingOpt>(std::move(out), caster);
      }

      return detail::cast_safe<StringEncodingOpt>(std::move(out));
    } catch (const std::exception &Exc) {
      fatalError("Error in obfuscate_string: '"s + Exc.what() + "'");
    }
  }
  return StringEncOptSkip();
}

BreakControlFlowOpt PyObfuscationConfig::breakControlFlow(llvm::Module *M,
                                                          llvm::Function *F) {
  py::gil_scoped_acquire gil;
  const auto *Base = static_cast<const ObfuscationConfig *>(this);
  py::function override = py::get_override(Base, "break_control_flow");
  if (override) {
    try {
      py::object out = override(M, F);
      if (out.is_none())
        return false;

      if (py::isinstance<py::bool_>(out)) {
        bool Value = out.cast<py::bool_>();
        return Value;
      }

      bool is_temp =
          detail::cast_is_temporary_value_reference<BreakControlFlowOpt>::value;
      if (is_temp) {
        static detail::override_caster_t<BreakControlFlowOpt> caster;
        return detail::cast_ref<BreakControlFlowOpt>(std::move(out), caster);
      }

      return detail::cast_safe<BreakControlFlowOpt>(std::move(out));
    } catch (const std::exception &Exc) {
      fatalError("Error in break_control_flow: '"s + Exc.what() + "'");
    }
  }
  return false;
}

ControlFlowFlatteningOpt
PyObfuscationConfig::controlFlowGraphFlattening(llvm::Module *M,
                                                llvm::Function *F) {
  py::gil_scoped_acquire gil;
  const auto *Base = static_cast<const ObfuscationConfig *>(this);
  py::function override = py::get_override(Base, "flatten_cfg");
  if (override) {
    try {
      py::object out = override(M, F);
      if (out.is_none())
        return false;

      if (py::isinstance<py::bool_>(out)) {
        bool Value = out.cast<py::bool_>();
        return Value;
      }

      bool is_temp = detail::cast_is_temporary_value_reference<
          ControlFlowFlatteningOpt>::value;
      if (is_temp) {
        static detail::override_caster_t<ControlFlowFlatteningOpt> caster;
        return detail::cast_ref<ControlFlowFlatteningOpt>(std::move(out),
                                                          caster);
      }

      return detail::cast_safe<ControlFlowFlatteningOpt>(std::move(out));
    } catch (const std::exception &Exc) {
      fatalError("Error in flatten_cfg: '"s + Exc.what() + "'");
    }
  }
  return false;
}

StructAccessOpt
PyObfuscationConfig::obfuscateStructAccess(llvm::Module *M, llvm::Function *F,
                                           llvm::StructType *S) {
  py::gil_scoped_acquire gil;
  const auto *Base = static_cast<const ObfuscationConfig *>(this);
  py::function override = py::get_override(Base, "obfuscate_struct_access");
  if (override) {
    try {
      py::object out = override(M, F, S);
      if (out.is_none())
        return false;

      if (py::isinstance<py::bool_>(out)) {
        bool Value = out.cast<py::bool_>();
        return Value;
      }

      bool is_temp =
          detail::cast_is_temporary_value_reference<StringEncodingOpt>::value;
      if (is_temp) {
        static detail::override_caster_t<StructAccessOpt> caster;
        return detail::cast_ref<StructAccessOpt>(std::move(out), caster);
      }

      return detail::cast_safe<StructAccessOpt>(std::move(out));
    } catch (const std::exception &Exc) {
      fatalError("Error in obfuscate_struct_access: '"s + Exc.what() + "'");
    }
  }
  return false;
}

VarAccessOpt
PyObfuscationConfig::obfuscateVariableAccess(llvm::Module *M, llvm::Function *F,
                                             llvm::GlobalVariable *GV) {
  py::gil_scoped_acquire gil;
  const auto *Base = static_cast<const ObfuscationConfig *>(this);
  py::function override = py::get_override(Base, "obfuscate_variable_access");
  if (override) {
    try {
      py::object out = override(M, F, GV);
      if (out.is_none())
        return false;

      if (py::isinstance<py::bool_>(out)) {
        bool val = out.cast<py::bool_>();
        return val;
      }

      if (detail::cast_is_temporary_value_reference<VarAccessOpt>::value) {
        static detail::override_caster_t<VarAccessOpt> caster;
        return detail::cast_ref<VarAccessOpt>(std::move(out), caster);
      }

      return detail::cast_safe<VarAccessOpt>(std::move(out));
    } catch (const std::exception &Exc) {
      fatalError("Error in obfuscate_variable_access: '"s + Exc.what() + "'");
    }
  }
  return false;
}

AntiHookOpt PyObfuscationConfig::antiHooking(llvm::Module *M,
                                             llvm::Function *F) {
  py::gil_scoped_acquire gil;
  const auto *Base = static_cast<const ObfuscationConfig *>(this);
  py::function override = py::get_override(Base, "anti_hooking");
  if (override) {
    try {
      py::object out = override(M, F);
      if (out.is_none())
        return false;

      if (py::isinstance<py::bool_>(out)) {
        bool Res = out.cast<py::bool_>();
        return Res;
      }

      if (detail::cast_is_temporary_value_reference<AntiHookOpt>::value) {
        static detail::override_caster_t<AntiHookOpt> caster;
        return detail::cast_ref<AntiHookOpt>(std::move(out), caster);
      }

      return detail::cast_safe<AntiHookOpt>(std::move(out));
    } catch (const std::exception &Exc) {
      fatalError("Error in anti_hooking: '"s + Exc.what() + "'");
    }
  }
  return false;
}

bool PyObfuscationConfig::hasReportDiffOverride() {
  std::call_once(OverridesReportDiffChecked, [this]() {
    const auto *Base = static_cast<const ObfuscationConfig *>(this);
    OverridesReportDiff =
        static_cast<bool>(py::get_override(Base, "report_diff"));
  });
  return OverridesReportDiff;
}

void PyObfuscationConfig::reportDiff(const std::string &Pass,
                                     const std::string &Original,
                                     const std::string &Obfuscated) {
  if (hasReportDiffOverride()) {
    py::gil_scoped_acquire gil;
    const auto *Base = static_cast<const ObfuscationConfig *>(this);
    py::function override = py::get_override(Base, "report_diff");
    assert(override && "Checked once in ctor");
    try {
      override(Pass, Original, Obfuscated);
    } catch (const std::exception &Exc) {
      fatalError("Error in report_diff: '"s + Exc.what() + "'");
    }
  }
}

ArithmeticOpt PyObfuscationConfig::obfuscateArithmetics(llvm::Module *M,
                                                        llvm::Function *F) {
  py::gil_scoped_acquire gil;
  const auto *Base = static_cast<const ObfuscationConfig *>(this);
  py::function override = py::get_override(Base, "obfuscate_arithmetic");
  if (override) {
    try {
      py::object out = override(M, F);
      if (out.is_none())
        return false;

      if (py::isinstance<py::bool_>(out)) {
        bool res = out.cast<py::bool_>();
        return res;
      }

      if (detail::cast_is_temporary_value_reference<ArithmeticOpt>::value) {
        static detail::override_caster_t<ArithmeticOpt> caster;
        return detail::cast_ref<ArithmeticOpt>(std::move(out), caster);
      }

      return detail::cast_safe<ArithmeticOpt>(std::move(out));
    } catch (const std::exception &Exc) {
      fatalError("Error in obfuscate_arithmetic: '"s + Exc.what() + "'");
    }
  }
  return false;
}

OpaqueConstantsOpt PyObfuscationConfig::obfuscateConstants(llvm::Module *M,
                                                           llvm::Function *F) {
  py::gil_scoped_acquire gil;
  const auto *Base = static_cast<const ObfuscationConfig *>(this);
  py::function override = py::get_override(Base, "obfuscate_constants");
  if (override) {
    try {
      py::object out = override(M, F);
      if (out.is_none())
        return OpaqueConstantsSkip();

      if (py::isinstance<py::bool_>(out)) {
        bool Res = out.cast<py::bool_>();
        return OpaqueConstantsBool(Res);
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

      bool is_temp =
          detail::cast_is_temporary_value_reference<OpaqueConstantsOpt>::value;
      if (is_temp) {
        static detail::override_caster_t<OpaqueConstantsOpt> caster;
        return detail::cast_ref<OpaqueConstantsOpt>(std::move(out), caster);
      }

      return detail::cast_safe<OpaqueConstantsOpt>(std::move(out));
    } catch (const std::exception &Exc) {
      fatalError("Error in obfuscate_constants: '"s + Exc.what() + "'");
    }
  }
  return OpaqueConstantsSkip();
}

} // end namespace omvll

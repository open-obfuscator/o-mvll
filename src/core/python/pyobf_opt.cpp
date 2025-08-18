//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "omvll/passes/ObfuscationOpt.hpp"

#include "init.hpp"

namespace py = pybind11;

using namespace pybind11::literals;

namespace omvll {

py::module_ &py_init_obf_opt(py::module_ &m) {
  // clang-format off
  // Strings Encoding
  py::class_<StringEncOptSkip>(m, "StringEncOptSkip",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_string` protection.

    This option can be used to **not** protect the string given in the callback's parameters.
    )delim")
    .def(py::init<>());

  py::class_<StringEncOptGlobal>(m, "StringEncOptGlobal",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_string` protection.

    This option protect the string in a global constructor.

    .. warning::

      With this option, the string will be in clear as soon as the binary is loaded.
    )delim")
    .def(py::init<>());

  py::class_<StringEncOptDefault>(m, "StringEncOptDefault",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_string` protection.

    Option that defers the choice of the protection to O-MVLL.
    )delim")
    .def(py::init<>());

  py::class_<StringEncOptLocal>(m, "StringEncOptLocal",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_string` protection.

    This option protects the string lazily when used within the function.

    .. danger::

        For large strings, this option can introduce a **huge** overhead if the `loopThreshold` is not used.

    )delim")
    .def(py::init<>());

  py::class_<StringEncOptReplace>(m, "StringEncOptReplace",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_string` protection.

    This option determines the new string that replaces the one from the parameter
    )delim")
    .def(py::init<>(), "Construct an empty string")
    .def(py::init<std::string>(), "new_string"_a);

  // Opaque Field Access
  py::class_<StructAccessOpt>(m, "StructAccessOpt",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_struct_access` protection.

    This boolean option determines whether the protection must be enabled (e.g. ``StructAccessOpt(True)``)
    )delim")
    .def(py::init<bool>());

  py::class_<VarAccessOpt>(m, "VarAccessOpt",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_variable_access` protection.

    This boolean option determines whether the protection must be enabled (e.g. ``VarAccessOpt(True)``)
    )delim")
    .def(py::init<bool>());

  // Break Control Flow
  py::class_<BreakControlFlowOpt>(m, "BreakControlFlowOpt",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.break_control_flow` protection.

    This boolean option determines whether the protection must be enabled (e.g. ``BreakControlFlowOpt(True)``)
    )delim")
    .def(py::init<bool>());

  // CFG Flattening
  py::class_<ControlFlowFlatteningOpt>(m, "ControlFlowFlatteningOpt",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.flatten_cfg` protection.

    This boolean option determines whether the protection must be enabled (e.g. ``ControlFlowFlatteningOpt(False)``)
    )delim")
    .def(py::init<bool>(), "value"_a);

  // Anti-Hooking
  py::class_<AntiHookOpt>(m, "AntiHookOpt",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.anti_hooking` protection.

    This option only accepts a boolean value (e.g. ``AntiHookOpt(True)``)
    )delim")
    .def(py::init<bool>(), "value"_a);

  // MBA
  py::class_<ArithmeticOpt>(m, "ArithmeticOpt",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_arithmetic` protection.

    This option defines the number of rounds to transform arithmetic expressions (e.g. ``ArithmeticOpt(3)``).
    It also accepts a boolean value which defers the number of rounds to O-MVLL (e.g. ``ArithmeticOpt(True)``).
    )delim")
    .def(py::init<uint8_t>(), "rounds"_a)
    .def(py::init<bool>(),    "value"_a);

  // Opaque Constants
  py::class_<OpaqueConstantsLowerLimit>(m, "OpaqueConstantsLowerLimit",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_constants` protection.

    This option defines lower limit from which constants must be obfuscated (e.g. ``OpaqueConstantsLowerLimit(100)``)
    )delim")
    .def(py::init<uint64_t>(), "limit"_a);

  py::class_<OpaqueConstantsBool>(m, "OpaqueConstantsBool",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_constants` protection.

    This option defines whether or not the constants must be obfuscated. If the value is set to `False`,
    the constants are not protected otherwise, **all** the constants are protected.
    )delim")
    .def(py::init<bool>(), "value"_a);

  py::class_<OpaqueConstantsSkip>(m, "OpaqueConstantsSkip",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_constants` protection.

    Alias for ``OpaqueConstantsBool(False)``
    )delim")
    .def(py::init<>());

  py::class_<OpaqueConstantsSet>(m, "OpaqueConstantsSet",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.obfuscate_constants` protection.

    This option takes a list of constants that must be protected by the pass
    (e.g. ``OpaqueConstantsSet([0x12234, 1, 2])``)
    )delim")
    .def(py::init<std::vector<uint64_t>>(), "constants"_a);

  // Indirect Branch
  py::class_<IndirectBranchOpt>(m, "IndirectBranchOpt",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.indirect_branch` protection.

    This option only accepts a boolean value (e.g. ``IndirectBranchOpt(True)``)
    )delim")
    .def(py::init<bool>(), "value"_a);

  // Indirect Call
  py::class_<IndirectCallOpt>(m, "IndirectCallOpt",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.indirect_call` protection.

    This option only accepts a boolean value (e.g. ``IndirectCallOpt(True)``)
    )delim")
    .def(py::init<bool>(), "value"_a);

  // BasicBlock Duplicate
  py::class_<BasicBlockDuplicateSkip>(m, "BasicBlockDuplicateSkip",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.basic_block_duplicate` protection.

    Alias for not enabling BasicBlockDuplicate.
    )delim")
    .def(py::init<>());

  py::class_<BasicBlockDuplicateWithProbability>(m, "BasicBlockDuplicateWithProbability",
    R"delim(
    Option for the :meth:`omvll.ObfuscationConfig.basic_block_duplicate` protection.

    This option defines a probability to be used when choosing basic blocks to be duplicated.
    For example, ``BasicBlockDuplicateWithProbability(0)`` implies that the pass never runs,
    whereas ``BasicBlockDuplicateWithProbability(100)`` duplicates all basic blocks in a
    function.
    )delim")
    .def(py::init<unsigned>(), "probability"_a);

  return m;
  // clang-format on
}

} // end namespace omvll

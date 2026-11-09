//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "omvll/passes/ObfuscationOpt.hpp"
#include <pybind11/pytypes.h>

#include "init.hpp"

namespace py = pybind11;

using namespace pybind11::literals;

namespace omvll {

py::module_ &py_init_obf_opt(py::module_ &m) {
  // clang-format off
  // Strings Encoding
  py::class_<StringEncOptSkip>(m, "StringEncOptSkip",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_string`
    callback. Leaves the string unprotected.
    )delim")
    .def(py::init<>());

  py::class_<StringEncOptDefault>(m, "StringEncOptDefault",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_string`
    callback. Defers the choice of encoding strategy to O-MVLL.
    )delim")
    .def(py::init<>());

  py::class_<StringEncOptGlobal>(m, "StringEncOptGlobal",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_string`
    callback. Decodes the string in a global constructor (before ``main``).

    .. warning::

       The string is briefly visible in clear memory as soon as the binary
       is loaded.
    )delim")
    .def(py::init<>());

  py::class_<StringEncOptLocal>(m, "StringEncOptLocal",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_string`
    callback. Decodes the string lazily at the point of use within the
    function.

    .. danger::

       For large strings this can introduce significant overhead if called
       in a loop.
    )delim")
    .def(py::init<>());

  py::class_<StringEncOptReplace>(m, "StringEncOptReplace",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_string`
    callback. Replaces the original string with *new_string*.

    :param new_string: The replacement string. Defaults to an empty string.
    :type new_string: str
    )delim")
    .def(py::init<>())
    .def(py::init<std::string>(), "new_string"_a);

  // Opaque Field Access
  py::class_<StructAccessOpt>(m, "StructAccessOpt",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_struct_access`
    callback.

    :param value: ``True`` enables the protection, ``False`` disables it.
    :type value: bool
    )delim")
    .def(py::init<bool>(), "value"_a);

  py::class_<VarAccessOpt>(m, "VarAccessOpt",
    R"delim(
    Option for the
    :py:meth:`~omvll.ObfuscationConfig.obfuscate_variable_access` callback.

    :param value: ``True`` enables the protection, ``False`` disables it.
    :type value: bool
    )delim")
    .def(py::init<bool>(), "value"_a);

  // Break Control Flow
  py::class_<BreakControlFlowOpt>(m, "BreakControlFlowOpt",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.break_control_flow`
    callback.

    :param value: ``True`` enables the protection, ``False`` disables it.
    :type value: bool
    )delim")
    .def(py::init<bool>(), "value"_a);

  // CFG Flattening
  py::class_<ControlFlowFlatteningOpt>(m, "ControlFlowFlatteningOpt",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.flatten_cfg` callback.

    :param value: ``True`` enables the protection, ``False`` disables it.
    :type value: bool
    )delim")
    .def(py::init<bool>(), "value"_a);

  // Anti-Hooking
  py::class_<AntiHookOpt>(m, "AntiHookOpt",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.anti_hooking` callback.

    :param value: ``True`` enables the protection, ``False`` disables it.
    :type value: bool
    )delim")
    .def(py::init<bool>(), "value"_a);

  // MBA
  py::class_<ArithmeticOpt>(m, "ArithmeticOpt",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_arithmetic`
    callback. Defines the number of rounds to apply to arithmetic expressions.

    :param rounds_or_value: Number of rounds (``int``, 0–255), or a boolean
       (``True`` uses O-MVLL's default of 3 rounds, ``False`` disables the pass).
    :type rounds_or_value: int or bool

    Examples::

       ArithmeticOpt(3)     # 3 explicit rounds
       ArithmeticOpt(True)  # O-MVLL default (3 rounds)
       ArithmeticOpt(False) # disabled
    )delim")
    .def(py::init<uint8_t>(), "rounds"_a)
    .def(py::init<bool>(),    "value"_a);

  // Opaque Constants
  py::class_<OpaqueConstantsLowerLimit>(m, "OpaqueConstantsLowerLimit",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_constants`
    callback. Obfuscates only constants whose value is at or above *limit*.

    :param limit: Lower bound; constants below this value are left unprotected.
    :type limit: int
    :param arith_rounds: Additional arithmetic obfuscation rounds. Default ``0``.
    :type arith_rounds: int

    Examples::

       OpaqueConstantsLowerLimit(100)
       OpaqueConstantsLowerLimit(100, arith_rounds=2)
    )delim")
    .def(py::init<uint64_t, uint8_t>(), "limit"_a, "arith_rounds"_a = 0);

  py::class_<OpaqueConstantsBool>(m, "OpaqueConstantsBool",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_constants`
    callback. Obfuscates all constants (``True``) or none (``False``).

    :param value: ``True`` protects all constants, ``False`` disables the pass.
    :type value: bool
    :param arith_rounds: Additional arithmetic obfuscation rounds applied to
       the generated opaque expressions. Default ``0``.
    :type arith_rounds: int
    )delim")
    .def(py::init<bool, uint8_t>(), "value"_a, "arith_rounds"_a = 0);

  py::class_<OpaqueConstantsSkip>(m, "OpaqueConstantsSkip",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_constants`
    callback. Disables the pass for the current function. Alias for
    :py:class:`~omvll.OpaqueConstantsBool`\(``False``).
    )delim")
    .def(py::init<>());

  py::class_<OpaqueConstantsSet>(m, "OpaqueConstantsSet",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_constants`
    callback. Obfuscates only the constants in the given list.

    :param constants: Specific constant values to protect.
    :type constants: list[int]
    :param arith_rounds: Additional arithmetic obfuscation rounds. Default ``0``.
    :type arith_rounds: int

    Examples::

       OpaqueConstantsSet([0x1234, 1, 2])
       OpaqueConstantsSet([1, 2], arith_rounds=2)
    )delim")
    .def(py::init<std::vector<uint64_t>, uint8_t>(), "constants"_a, "arith_rounds"_a = 0);

  py::class_<OpaqueConstantsExcludeSet>(m, "OpaqueConstantsExcludeSet",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.obfuscate_constants`
    callback. Obfuscates all constants **except** those in the given list.

    :param constants: Constant values to leave unprotected.
    :type constants: list[int]
    :param arith_rounds: Additional arithmetic obfuscation rounds. Default ``0``.
    :type arith_rounds: int

    Examples::

       OpaqueConstantsExcludeSet([0, 1])
       OpaqueConstantsExcludeSet([0, 1], arith_rounds=2)
    )delim")
    .def(py::init<std::vector<uint64_t>, uint8_t>(), "constants"_a, "arith_rounds"_a = 0);

  // Indirect Branch
  py::class_<IndirectBranchConfig>(m, "IndirectBranchConfig",
    R"delim(
    Configuration returned by the
    :py:meth:`~omvll.ObfuscationConfig.indirect_branch` callback.

    :param value: ``True`` enables the protection, ``False`` disables it.
    :type value: bool
    )delim")
    .def(py::init<bool>(), "value"_a);

  py::class_<IndirectBranchOpt>(m, "IndirectBranchOpt",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.indirect_branch`
    callback.

    :param value: ``True`` enables the protection, ``False`` disables it.
    :type value: bool
    )delim")
    .def(py::init<bool>(), "value"_a);

  // Indirect Call
  py::class_<IndirectCallConfig>(m, "IndirectCallConfig",
    R"delim(
    Configuration returned by the
    :py:meth:`~omvll.ObfuscationConfig.indirect_call` callback.

    :param value: ``True`` enables the protection, ``False`` disables it.
    :type value: bool
    )delim")
    .def(py::init<bool>(), "value"_a);

  py::class_<IndirectCallOpt>(m, "IndirectCallOpt",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.indirect_call` callback.

    :param value: ``True`` enables the protection, ``False`` disables it.
    :type value: bool
    )delim")
    .def(py::init<bool>(), "value"_a);

  // BasicBlock Duplicate
  py::class_<BasicBlockDuplicateSkip>(m, "BasicBlockDuplicateSkip",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.basic_block_duplicate`
    callback. Disables the pass for the current function.
    )delim")
    .def(py::init<>());

  py::class_<BasicBlockDuplicateWithProbability>(m, "BasicBlockDuplicateWithProbability",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.basic_block_duplicate`
    callback. Selects basic blocks to duplicate with the given probability.

    :param probability: Percentage chance (0–100) for each basic block to be
       duplicated. ``0`` means never, ``100`` duplicates every block.
    :type probability: int
    )delim")
    .def(py::init<unsigned>(), "probability"_a);

  // BasicBlock Split
  py::class_<BasicBlockSplitSkip>(m, "BasicBlockSplitSkip",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.basic_block_split`
    callback. Disables the pass for the current function.
    )delim")
    .def(py::init<>());

  py::class_<BasicBlockSplitWithProbability>(m, "BasicBlockSplitWithProbability",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.basic_block_split`
    callback. Selects basic blocks to split in half with the given probability.

    :param probability: Percentage chance (0–100) for each basic block to be
       split. ``0`` means never, ``100`` splits every eligible block.
    :type probability: int
    )delim")
    .def(py::init<unsigned>(), "probability"_a);
  
  // Function Outline
  py::class_<FunctionOutlineSkip>(m, "FunctionOutlineSkip",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.function_outline`
    callback. Disables the pass for the current function.
    )delim")
    .def(py::init<>());

  py::class_<FunctionOutlineWithProbability>(m, "FunctionOutlineWithProbability",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.function_outline`
    callback. Selects basic blocks to outline into new functions with the
    given probability.

    :param probability: Percentage chance (0–100) for each candidate block to
       be outlined. ``0`` means never, ``100`` outlines every candidate.
    :type probability: int
    )delim")
    .def(py::init<unsigned>(), "probability"_a);

  // ShuffleOps
  py::class_<ShuffleOpsOpt>(m, "ShuffleOpsOpt",
    R"delim(
    Option for the :py:meth:`~omvll.ObfuscationConfig.shuffle_ops` callback.
    Controls instruction-level reordering within basic blocks.

    :param min_block_size: Minimum number of movable instructions a block must
                           contain before shuffling is attempted. Blocks with
                           fewer movable instructions are left untouched.
                           Defaults to 4.
    :type min_block_size: int
    )delim")
    .def(py::init<uint64_t>(), "min_block_size"_a = ShuffleOpsOpt::DefaultMinBlockSize);

  return m;
  // clang-format on
}

} // end namespace omvll

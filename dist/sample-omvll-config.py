# See the API documentation at https://obfuscator.re/o-mvll/
import omvll
from functools import lru_cache

THIRD_PARTY_MODULES = []

EXCLUDED_FUNCTIONS = []

SENSITIVE_FUNCTIONS = []

class MyConfig(omvll.ObfuscationConfig):

    omvll.config.global_mod_exclude = THIRD_PARTY_MODULES
    omvll.config.global_func_exclude = EXCLUDED_FUNCTIONS
    omvll.config.shuffle_functions = True
    omvll.config.inline_jni_wrappers = True

    def __init__(self):
        super().__init__()

    def obfuscate_string(self, mod: omvll.Module, func: omvll.Function,
                         string: bytes):
        return omvll.StringEncOptGlobal()

    def obfuscate_constants(self, mod: omvll.Module, func: omvll.Function):
        return omvll.OpaqueConstantsBool(True)

    def obfuscate_arithmetic(self, mod: omvll.Module, func: omvll.Function):
        return omvll.ArithmeticOpt(2)

    def obfuscate_variable_access(self, mod: omvll.Module, func: omvll.Function,
                                  var: omvll.GlobalVariable):
        return omvll.VarAccessOpt(
            omvll.ObfuscationConfig.default_config(
                self, mod, func, [], [], SENSITIVE_FUNCTIONS, 10))

    def obfuscate_struct_access(self, mod: omvll.Module, func: omvll.Function,
                                struct: omvll.Struct):
        return omvll.StructAccessOpt(
            omvll.ObfuscationConfig.default_config(
                self, mod, func, [], [], SENSITIVE_FUNCTIONS, 10))

    def flatten_cfg(self, mod: omvll.Module, func: omvll.Function):
        return omvll.ControlFlowFlatteningOpt(
            omvll.ObfuscationConfig.default_config(
                self, mod, func, [], [], SENSITIVE_FUNCTIONS, 10))

    def break_control_flow(self, mod: omvll.Module, func: omvll.Function):
        return omvll.BreakControlFlowOpt(
            omvll.ObfuscationConfig.default_config(
                self, mod, func, [], [], SENSITIVE_FUNCTIONS, 5))

    def basic_block_duplicate(self, mod: omvll.Module, func: omvll.Function):
        return omvll.BasicBlockDuplicateWithProbability(5)

    def basic_block_split(self, mod: omvll.Module, func: omvll.Function):
        return omvll.BasicBlockSplitWithProbability(10)

    def function_outline(self, mod: omvll.Module, func: omvll.Function):
        return omvll.FunctionOutlineWithProbability(5)

    def shuffle_ops(self, mod: omvll.Module, func: omvll.Function):
        return omvll.ShuffleOpsOpt(min_block_size=4)

    def indirect_branch(self, mod: omvll.Module, func: omvll.Function):
        return omvll.ObfuscationConfig.default_config(
            self, mod, func, [], [], SENSITIVE_FUNCTIONS, 5)

    def indirect_call(self, mod: omvll.Module, func: omvll.Function):
        return omvll.ObfuscationConfig.default_config(
            self, mod, func, [], [], SENSITIVE_FUNCTIONS, 5)

    def anti_hooking(self, mod: omvll.Module, func: omvll.Function):
        return omvll.AntiHookOpt(
            omvll.ObfuscationConfig.default_config(
                self, mod, func, [], [], SENSITIVE_FUNCTIONS, 0))


@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    return MyConfig()

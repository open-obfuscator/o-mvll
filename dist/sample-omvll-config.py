import omvll
from functools import lru_cache

class MyConfig(omvll.ObfuscationConfig):
    def __init__(self):
        super().__init__()

    def obfuscate_arithmetic(self, mod: omvll.Module,
                                   fun: omvll.Function) -> omvll.ArithmeticOpt:
        return True

    def flatten_cfg(self, mod: omvll.Module, func: omvll.Function):
        return True

    def obfuscate_string(self, _, __, string: bytes):
        return omvll.StringEncOptGlobal()

@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    """
    Return an instance of `ObfuscationConfig` which
    aims at describing the obfuscation scheme
    """
    return MyConfig()

#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import omvll
from functools import lru_cache

class MyConfig(omvll.ObfuscationConfig):
    excluded_function_value = "memcpy_xor"
    omvll.config.global_func_exclude = [excluded_function_value]

    def __init__(self):
        super().__init__()
    def obfuscate_arithmetic(self, mod: omvll.Module,
                                   fun: omvll.Function) -> omvll.ArithmeticOpt:
        if (fun.name == self.excluded_function_value):
            print("obfuscate_arithmetic is called")
        return omvll.ArithmeticOpt(rounds=2)

@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    return MyConfig()

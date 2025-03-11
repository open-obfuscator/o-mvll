#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import omvll
from functools import lru_cache

class MyConfig(omvll.ObfuscationConfig):
    def __init__(self):
        super().__init__()
    def obfuscate_arithmetic(self, mod: omvll.Module,
                                   fun: omvll.Function) -> omvll.ArithmeticOpt:
        return omvll.ArithmeticOpt(rounds=2) if  omvll.ObfuscationConfig.default_config(self, mod, fun, [], ["memcpy_xor"], [], 100) else False

@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    return MyConfig()

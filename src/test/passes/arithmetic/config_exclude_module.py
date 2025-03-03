#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import omvll
from functools import lru_cache

class MyConfig(omvll.ObfuscationConfig):
    omvll.config.global_mod_exclude = ["global-exclude-"]

    def __init__(self):
        super().__init__()
    def obfuscate_arithmetic(self, mod: omvll.Module,
                                   fun: omvll.Function) -> omvll.ArithmeticOpt:
        print("obfuscate_arithmetic is called")
        return omvll.ArithmeticOpt(rounds=2)

@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    return MyConfig()

#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import omvll
from difflib import unified_diff
from functools import lru_cache

class MyConfig(omvll.ObfuscationConfig):
    excluded_function_value = "check_password"
    omvll.config.global_func_exclude = [excluded_function_value]

    def __init__(self):
        super().__init__()
    def flatten_cfg(self, mod: omvll.Module, func: omvll.Function):
        if (func.name == self.excluded_function_value):
             print("flatten_cfg is called")
        return True

@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    return MyConfig()

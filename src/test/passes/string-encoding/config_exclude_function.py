#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import omvll
from functools import lru_cache

class MyConfig(omvll.ObfuscationConfig):
    excluded_function_value = "check_code"
    omvll.config.global_func_exclude = [excluded_function_value]

    def __init__(self):
        super().__init__()
    def obfuscate_string(self, _, __, string: bytes):
        if string.endswith(b"ObfuscateMe"):
            print("string-encoding is called")
            return omvll.StringEncOptGlobal()

@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    return MyConfig()

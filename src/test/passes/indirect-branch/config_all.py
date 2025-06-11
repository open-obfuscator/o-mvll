#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import omvll
from functools import lru_cache

class MyConfig(omvll.ObfuscationConfig):
    omvll.config.shuffle_functions = False

    def __init__(self):
        super().__init__()
    def indirect_branch(self, mod: omvll.Module, func: omvll.Function):
        return True

@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    return MyConfig()

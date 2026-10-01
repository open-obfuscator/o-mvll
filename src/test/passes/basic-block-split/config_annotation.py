#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import omvll
from functools import lru_cache

class MyConfig(omvll.ObfuscationConfig):
    omvll.config.shuffle_functions = False

    def __init__(self):
        super().__init__()
    def basic_block_split(self, mod: omvll.Module, fun: omvll.Function):
        # Selection is driven purely by the source-level `bbsplit` / `!bbsplit`
        # annotations (baseline probability is 0, so unannotated functions are
        # left untouched and the negated annotation opts a function out).
        return (
            omvll.BasicBlockSplitWithProbability(100)
            if omvll.ObfuscationConfig.default_config(self, mod, fun, [], [], [], 0, "bbsplit")
            else omvll.BasicBlockSplitSkip()
        )

@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    return MyConfig()

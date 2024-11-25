#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import omvll
from functools import lru_cache

@lru_cache(maxsize=1)
def omvll_get_config():
    return omvll.ObfuscationConfig()

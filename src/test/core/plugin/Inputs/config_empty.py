import omvll
from functools import lru_cache

@lru_cache(maxsize=1)
def omvll_get_config():
    return omvll.ObfuscationConfig()

import omvll
from functools import lru_cache

class MyConfig(omvll.ObfuscationConfig):
    def __init__(self):
        super().__init__()
    def obfuscate_string(self, _, __, string: bytes):
        if string.endswith(b"Hello"):
            return omvll.StringEncOptGlobal()
        if string.endswith(b".cpp"):
            return omvll.StringEncOptGlobal()
        if string.endswith(b"Swift"):
            return omvll.StringEncOptStack()

@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    return MyConfig()

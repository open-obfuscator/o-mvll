import omvll
from difflib import unified_diff
from functools import lru_cache

class MyConfig(omvll.ObfuscationConfig):
    def __init__(self):
        super().__init__()
    def obfuscate_arithmetic(self, mod: omvll.Module,
                                   fun: omvll.Function) -> omvll.ArithmeticOpt:
        return omvll.ArithmeticOpt(rounds=2)
    def report_diff(self, pass_name: str, original: str, obfuscated: str):
        print(pass_name, "applied obfuscation:")
        diff = unified_diff(original.splitlines(), obfuscated.splitlines(),
                            'original', 'obfuscated', lineterm='')
        for line in diff:
            print(line)

@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    return MyConfig()

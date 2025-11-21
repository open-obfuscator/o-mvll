#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import omvll
from functools import lru_cache
from pathlib import Path

class MyConfig(omvll.ObfuscationConfig):
    current_path = Path(__file__).resolve().parent
    omvll.config.output_folder = str(current_path) + "/tmp/"

@lru_cache(maxsize=1)
def omvll_get_config() -> omvll.ObfuscationConfig:
    return MyConfig()

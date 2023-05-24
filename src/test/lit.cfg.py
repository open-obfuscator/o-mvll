import os
import lit
from lit.llvm import llvm_config

config.name = "O-MVLL Tests"
config.suffixes = ['.c', '.cpp']
config.test_format = lit.formats.ShTest(True)
config.test_source_root = os.path.dirname(__file__)

# Enable tests based on required targets, e.g. REQUIRES: x86-registered-target
llvm_config.feature_config([('--targets-built',
                             lambda s: [arch.lower() + '-registered-target' for arch in s.split()])])

# The tools directory defaults to LLVM_BINARY_DIR. In order to run tests with a different compiler,
# pass the installation base path via LLVM_TOOLS_DIR at configuration time explicitly.
print("Running tests with:", os.path.join(config.llvm_tools_dir, 'clang'))
llvm_config.add_tool_substitutions(["clang", "clang++"], config.llvm_tools_dir)
llvm_config.add_tool_substitutions(["FileCheck", "count", "not"], config.llvm_tools_dir)

# The plugin is a shared library in our build-tree
plugin_file = os.path.join(config.omvll_plugin_dir, 'libOMVLL' + config.llvm_plugin_suffix)
print("Testing plugin file:", plugin_file)
config.substitutions.append(('%libOMVLL', plugin_file))

# We need this to find the Python standard library
if 'OMVLL_PYTHONPATH' in os.environ:
    print("OMVLL_PYTHONPATH:", os.environ['OMVLL_PYTHONPATH'])
    config.environment['OMVLL_PYTHONPATH'] = os.environ['OMVLL_PYTHONPATH']
else:
    print("Please set the environment variable OMVLL_PYTHONPATH and try again, e.g.:")
    print("")
    print("  export OMVLL_PYTHONPATH=/path/to/Python-3.10.7/Lib")
    print("")
    print("For more info see: https://obfuscator.re/omvll/introduction/getting-started/#python-standard-library")
    exit(1)

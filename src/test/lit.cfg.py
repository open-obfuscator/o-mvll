#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import os
import lit
import sys
import platform
import subprocess
from lit.llvm import llvm_config

config.name = "O-MVLL Tests"
config.suffixes = ['.c', '.cpp', '.ll', '.m']
config.test_format = lit.formats.ShTest(True)
config.test_source_root = os.path.dirname(__file__)

# Enable tests based on required targets, e.g. REQUIRES: x86-registered-target
llvm_config.feature_config([('--targets-built',
                             lambda s: [arch.lower() + '-registered-target' for arch in s.split()])])

# Enable tests based on the OS of the host machine
if sys.platform.startswith('linux'):
    config.available_features.add('host-platform-linux')
if sys.platform == 'darwin':
    config.available_features.add('host-platform-macOS')
if sys.platform == 'win32':
    config.available_features.add('host-platform-windows')

# Allow tests to be based on the host architecture
host_arch = platform.machine()
if host_arch == 'x86_64':
    config.available_features.add('host-arch-x86')
elif host_arch == 'arm64':
    config.available_features.add('host-arch-arm64')

# For Android tests, use clang from AndroidNDK.
if config.omvll_plugin_abi == 'Android' or \
   config.omvll_plugin_abi == 'CustomAndroid':
    config.available_features.add('android_abi')

    clang_exe = os.path.join(config.llvm_bin_dir, 'clang')
    if not os.path.exists(clang_exe):
        print("clang compiler not found:", clang_exe)
        exit(1)
    print("Testing compiler:", clang_exe)
    clang_path = config.llvm_bin_dir

# For iOS tests, always use Apple Clang as default compiler.
if config.omvll_plugin_abi == 'Apple':
    config.available_features.add('apple_abi')

    try:
        xcode_path = subprocess.check_output(['xcode-select', '-p'], stderr=subprocess.PIPE).strip().decode()
    except (subprocess.CalledProcessError, OSError):
        print("xcode-select not found. Please install Xcode.")
        exit(1)
    clang_path = os.path.join(xcode_path, 'Toolchains/XcodeDefault.xctoolchain/usr/bin')
    clang_exe = os.path.join(clang_path, 'clang')
    print("Testing compiler:", clang_exe)

    try:
        cmd = ["xcrun", "--show-sdk-path", "--sdk", "iphoneos"]
        ios_sdk = subprocess.check_output(cmd, stderr=subprocess.PIPE).strip().decode()
    except (subprocess.CalledProcessError, OSError):
        print("xcrun not found. Please run command: xcode-select --install")
        exit(1)
    print("Using iOS SDK:", ios_sdk)
    config.substitutions.append(('%EXTRA_CC_FLAGS', f"-isysroot {ios_sdk}"))

llvm_config.add_tool_substitutions(["clang", "clang++"], clang_path)
llvm_config.add_tool_substitutions(["FileCheck", "count", "not"], config.llvm_tools_dir)

# The plugin is a shared library in our build-tree
plugin_file = os.path.join(config.omvll_plugin_dir, 'libOMVLL' + config.llvm_plugin_suffix)
print("Testing plugin file:", plugin_file)
config.substitutions.append(('%libOMVLL', plugin_file))

print("Available features are:", config.available_features)

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

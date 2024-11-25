//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// Prepare test output dir with plugin, yml-config, py-config and fake python-libs dir
// RUN: rm -rf %T_plugindir
// RUN: mkdir -p %T_plugindir
// RUN: mkdir -p %T_plugindir/Python-3.10.7/Lib
// RUN: cp %libOMVLL %T_plugindir/libOMVLL.so
// RUN: cp %S/Inputs/omvll.yml %T_plugindir/omvll.yml
// RUN: cp %S/Inputs/config_empty.py %T_plugindir/config_empty.py

// Run clang from a different CWD and pass plugin from the test output dir
// RUN: cd /tmp
// RUN: rm -rf omvll.log omvll.yml
// RUN: clang -target aarch64-linux-android -fpass-plugin=%T_plugindir/libOMVLL.so -S %s -o /dev/null
// RUN: FileCheck %s -DPLUGIN_DIR=%T_plugindir < omvll.log

// Check that we load the yml-config from the test output dir
// CHECK: Looking for omvll.yml in /tmp
// CHECK: Looking for omvll.yml in /
// CHECK: Looking for omvll.yml in [[PLUGIN_DIR]]
// CHECK: Loading omvll.yml from [[PLUGIN_DIR]]/omvll.yml
// CHECK: OMVLL_PYTHONPATH = Python-3.10.7/Lib
// CHECK: OMVLL_CONFIG = config_empty.py

void test() {}

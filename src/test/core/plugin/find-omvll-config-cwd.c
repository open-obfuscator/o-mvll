// Prepare test output dir with yml-config, py-config and fake python-libs dir
// RUN: rm -rf %T_cwd
// RUN: mkdir -p %T_cwd
// RUN: mkdir -p %T_cwd/Python-3.10.7/Lib
// RUN: cp %S/Inputs/omvll.yml %T_cwd/omvll.yml
// RUN: cp %S/Inputs/config_empty.py %T_cwd/config_empty.py

// Run clang from the test output dir
// RUN: cd %T_cwd
// RUN: clang -target aarch64-linux-android -fno-legacy-pass-manager -fpass-plugin=%libOMVLL -S %s -o /dev/null
// RUN: FileCheck %s -DCWD=%T_cwd < omvll.log

// Check that we load the yml-config from the test output dir
// CHECK: Looking for omvll.yml in [[CWD]]
// CHECK: Loading omvll.yml from [[CWD]]/omvll.yml
// CHECK: OMVLL_PYTHONPATH = Python-3.10.7/Lib
// CHECK: OMVLL_CONFIG = config_empty.py

void test() {}

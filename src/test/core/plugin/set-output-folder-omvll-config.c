//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// Run clang from the test output dir
// RUN: env OMVLL_CONFIG=%S/Inputs/config_output_folder.py clang -target aarch64-linux-android -fpass-plugin=%libOMVLL -S %s -o /dev/null
// RUN: FileCheck %s -DCWD=%T_cwd < %S/Inputs/tmp/omvll-logs/omvll-init.log

// Check that we load the yml-config from the test output dir
// CHECK: Found OMVLL at: {{.*}}/Inputs/config_output_folder.py

// RUN: rm -rf  %S/Inputs/tmp

void test() {}

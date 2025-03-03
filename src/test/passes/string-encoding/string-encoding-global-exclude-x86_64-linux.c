//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: x86-registered-target

// RUN: env OMVLL_CONFIG=%S/config_exclude_module.py   clang -target x86_64-pc-linux-gnu  -fpass-plugin=%libOMVLL -O1 -S %s -o /dev/null | FileCheck --allow-empty --check-prefix=CHECK %s
// RUN: env OMVLL_CONFIG=%S/config_exclude_function.py clang -target x86_64-pc-linux-gnu  -fpass-plugin=%libOMVLL -O1 -S %s -o /dev/null | FileCheck --allow-empty --check-prefix=CHECK %s

// Make sure the string is not encoded.
// CHECK-NOT: obfuscate_string is called

void check_code(int code) {
  char *myString = "ObfuscateMe";
  if (myString[0] == 'O') {
    myString[0] = 'N';
  }
}

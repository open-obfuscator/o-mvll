//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: x86-registered-target

// RUN: env OMVLL_CONFIG=%S/config_exclude_module.py   clang -target x86_64-pc-linux-gnu  -fpass-plugin=%libOMVLL -O1 -S %s -o /dev/null | FileCheck --allow-empty --check-prefix=CHECK %s
// RUN: env OMVLL_CONFIG=%S/config_exclude_function.py clang -target x86_64-pc-linux-gnu  -fpass-plugin=%libOMVLL -O1 -S %s -o /dev/null | FileCheck --allow-empty --check-prefix=CHECK %s

// Make sure the function is not flattened.
// CHECK-NOT: flatten_cfg is called

int check_password(const char *passwd) {
  if (passwd[0] == 'r') {
    if (passwd[1] == 'i') {
      if (passwd[2] == 'g') {
        if (passwd[3] == 'h') {
          if (passwd[4] == 't') {
            return 0;
          }
        }
      }
    }
  }
  return 1;
}

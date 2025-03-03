//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: x86-registered-target

// RUN: env OMVLL_CONFIG=%S/config_exclude_module.py   clang -target x86_64-pc-linux-gnu  -fpass-plugin=%libOMVLL -O1 -S %s -o /dev/null | FileCheck --allow-empty --check-prefix=CHECK %s
// RUN: env OMVLL_CONFIG=%S/config_exclude_function.py clang -target x86_64-pc-linux-gnu  -fpass-plugin=%libOMVLL -O1 -S %s -o /dev/null | FileCheck --allow-empty --check-prefix=CHECK %s

// Make sure the obfuscate arithmetic function was NOT called
// CHECK-NOT: obfuscate_arithmetic is called

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

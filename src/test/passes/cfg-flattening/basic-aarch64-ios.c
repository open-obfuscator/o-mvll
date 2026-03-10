//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: aarch64-registered-target && apple_abi

// RUN:                                   clang -target arm64-apple-ios                          -O1 -fno-verbose-asm -S %s -o -
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target arm64-apple-ios  -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=FLAT-IOS %s

// Check for jump table targets setup at the beginning of the function.

// FLAT-IOS-LABEL:        check_password:
// FLAT-IOS:                mov     w19, #53399
// FLAT-IOS-NEXT:           movk    w19, #26689, lsl #16
// FLAT-IOS-NEXT:           mov     w20, #41365
// FLAT-IOS-NEXT:           movk    w20, #6014, lsl #16
// FLAT-IOS-NEXT:           mov     w21, #52826
// FLAT-IOS-NEXT:           movk    w21, #7648, lsl #16
// FLAT-IOS-NEXT:           mov     w22, #35197
// FLAT-IOS-NEXT:           movk    w22, #63120, lsl #16
// FLAT-IOS-NEXT:           mov     w23, #53606
// FLAT-IOS-NEXT:           movk    w23, #26689, lsl #16
// FLAT-IOS-NEXT:           mov     w24, #52579
// FLAT-IOS-NEXT:           movk    w24, #7648, lsl #16
// FLAT-IOS-NEXT:           mov     w25, #44088
// FLAT-IOS-NEXT:           movk    w25, #2345, lsl #16
// FLAT-IOS-NEXT:           mov     w26, #10794
// FLAT-IOS-NEXT:           movk    w26, #56152, lsl #16
// FLAT-IOS-NEXT:           b       LBB0_4

int check_password(const char *passwd, unsigned len) {
  if (len != 5) {
    return 0;
  }
  if (passwd[0] == 'O') {
    if (passwd[1] == 'M') {
      if (passwd[2] == 'V') {
        if (passwd[3] == 'L') {
          if (passwd[4] == 'L') {
            return 1;
          }
        }
      }
    }
  }
  return 0;
}

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: aarch64-registered-target
// XFAIL: host-platform-linux

// RUN:                                   clang -target arm64-apple-ios                          -O1 -fno-verbose-asm -S %s -o -
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target arm64-apple-ios  -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=FLAT-IOS %s

// Check for jump table targets setup at the beginning of the function.

// FLAT-IOS-LABEL:        check_password:
// FLAT-IOS:                sub	sp, sp, #16
// FLAT-IOS:                mov	w19, #19350
// FLAT-IOS-NEXT:           movk	w19, #38589, lsl #16
// FLAT-IOS-NEXT:           mov	w20, #22813
// FLAT-IOS-NEXT:           movk	w20, #1156, lsl #16
// FLAT-IOS-NEXT:           mov	w21, #30177
// FLAT-IOS-NEXT:           movk	w21, #39846, lsl #16
// FLAT-IOS-NEXT:           mov	w22, #47812
// FLAT-IOS-NEXT:           movk	w22, #26529, lsl #16
// FLAT-IOS-NEXT:           mov	w23, #10194
// FLAT-IOS-NEXT:           movk	w23, #61753, lsl #16
// FLAT-IOS-NEXT:           mov	w24, #6121
// FLAT-IOS-NEXT:           movk	w24, #18322, lsl #16
// FLAT-IOS-NEXT:           mov	w25, #22795
// FLAT-IOS-NEXT:           movk	w25, #1156, lsl #16
// FLAT-IOS-NEXT:           mov	w26, #47852
// FLAT-IOS-NEXT:           movk	w26, #26529, lsl #16
// FLAT-IOS-NEXT:           b	LBB0_4

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

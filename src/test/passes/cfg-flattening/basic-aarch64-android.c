//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: aarch64-registered-target
// XFAIL: host-platform-macOS

// RUN:                                   clang -target aarch64-linux-android                         -O1 -fno-verbose-asm -S %s -o -
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target aarch64-linux-android -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=FLAT-ANDROID %s

// Check for jump table targets setup at the beginning of the function.

// FLAT-ANDROID-LABEL:    check_password:
// FLAT-ANDROID:            sub	sp, sp, #16
// FLAT-ANDROID:            movk	w4, #56095, lsl #16
// FLAT-ANDROID-NEXT:       movk	w5, #34774, lsl #16
// FLAT-ANDROID-NEXT:       movk	w6, #22523, lsl #16
// FLAT-ANDROID-NEXT:       movk	w7, #6598, lsl #16
// FLAT-ANDROID-NEXT:       mov	w19, #1
// FLAT-ANDROID-NEXT:       movk	w20, #54611, lsl #16
// FLAT-ANDROID-NEXT:       movk	w21, #56095, lsl #16
// FLAT-ANDROID-NEXT:       movk	w23, #22523, lsl #16
// FLAT-ANDROID-NEXT:       movk	w24, #48579, lsl #16
// FLAT-ANDROID-NEXT:       movk	w25, #30854, lsl #16
// FLAT-ANDROID-NEXT:       movk	w26, #22523, lsl #16
// FLAT-ANDROID-NEXT:       stur	w10, [x29, #-4]
// FLAT-ANDROID-NEXT:       b	.LBB0_5

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

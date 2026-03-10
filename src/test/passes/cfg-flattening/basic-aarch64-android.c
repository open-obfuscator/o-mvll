//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: aarch64-registered-target && android_abi

// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target aarch64-linux-android -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=FLAT-ANDROID %s

// Check for jump table targets setup at the beginning of the function.

// FLAT-ANDROID-LABEL:    check_password:
// FLAT-ANDROID:            movk    w4, #50251, lsl #16
// FLAT-ANDROID-NEXT:       movk    w5, #17606, lsl #16
// FLAT-ANDROID-NEXT:       movk    w6, #17961, lsl #16
// FLAT-ANDROID-NEXT:       movk    w7, #22555, lsl #16
// FLAT-ANDROID-NEXT:       movk    w19, #17606, lsl #16
// FLAT-ANDROID-NEXT:       movk    w20, #17961, lsl #16
// FLAT-ANDROID-NEXT:       movk    w21, #47252, lsl #16
// FLAT-ANDROID-NEXT:       movk    w22, #22555, lsl #16
// FLAT-ANDROID-NEXT:       movk    w23, #49424, lsl #16
// FLAT-ANDROID-NEXT:       movk    w24, #26706, lsl #16
// FLAT-ANDROID-NEXT:       movk    w25, #32064, lsl #16
// FLAT-ANDROID-NEXT:       movk    w26, #50251, lsl #16
// FLAT-ANDROID-NEXT:       stur    w11, [x29, #-4]
// FLAT-ANDROID-NEXT:       b       .LBB0_2

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

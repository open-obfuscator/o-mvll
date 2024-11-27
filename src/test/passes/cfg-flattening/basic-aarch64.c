//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: aarch64-registered-target

// RUN:                                   clang -target aarch64-linux-android                         -O1 -fno-verbose-asm -S %s -o -
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target aarch64-linux-android -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=FLAT-ANDROID %s

// RUN:                                   clang -target arm64-apple-ios                          -O1 -fno-verbose-asm -S %s -o -
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target arm64-apple-ios  -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=FLAT-IOS %s

// Check for jump table targets setup at the beginning of the function.

// FLAT-ANDROID-LABEL:    check_password:
// FLAT-ANDROID:            sub	sp, sp, #16
// FLAT-ANDROID:            movk	w4, #1631, lsl #16
// FLAT-ANDROID-NEXT:       movk	w5, #58789, lsl #16
// FLAT-ANDROID-NEXT:       movk	w6, #3991, lsl #16
// FLAT-ANDROID-NEXT:       movk	w7, #14783, lsl #16
// FLAT-ANDROID-NEXT:       movk	w19, #7850, lsl #16
// FLAT-ANDROID-NEXT:       movk	w20, #26243, lsl #16
// FLAT-ANDROID-NEXT:       movk	w21, #9442, lsl #16
// FLAT-ANDROID-NEXT:       movk	w22, #15558, lsl #16
// FLAT-ANDROID-NEXT:       movk	w23, #7850, lsl #16
// FLAT-ANDROID-NEXT:       movk	w24, #26243, lsl #16
// FLAT-ANDROID-NEXT:       movk	w25, #1631, lsl #16
// FLAT-ANDROID-NEXT:       stur	w10, [x29, #-4]
// FLAT-ANDROID-NEXT:       b	.LBB0_5

// FLAT-IOS-LABEL:        check_password:
// FLAT-IOS:                sub	sp, sp, #16
// FLAT-IOS:                mov	w19, #2181
// FLAT-IOS-NEXT:           movk	w19, #7850, lsl #16
// FLAT-IOS-NEXT:           mov	w20, #11601
// FLAT-IOS-NEXT:           movk	w20, #26243, lsl #16
// FLAT-IOS-NEXT:           mov	w21, #50895
// FLAT-IOS-NEXT:           movk	w21, #9442, lsl #16
// FLAT-IOS-NEXT:           mov	w22, #47994
// FLAT-IOS-NEXT:           movk	w22, #15558, lsl #16
// FLAT-IOS-NEXT:           mov	w23, #1804
// FLAT-IOS-NEXT:           movk	w23, #7850, lsl #16
// FLAT-IOS-NEXT:           mov	w24, #11818
// FLAT-IOS-NEXT:           movk	w24, #26243, lsl #16
// FLAT-IOS-NEXT:           mov	w25, #3737
// FLAT-IOS-NEXT:           movk	w25, #1631, lsl #16
// FLAT-IOS-NEXT:           b	LBB0_5

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

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: arm-registered-target

// RUN:                                   clang -target arm-linux-android                         -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=NO-FLAT %s
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target arm-linux-android -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=FLAT %s

// Check for jump table targets setup at the beginning of the function.

// NO-FLAT-LABEL: check_password:
// NO-FLAT:         mov	r2, #0
// NO-FLAT-NEXT:    cmp	r1, #5
// NO-FLAT-NEXT:    bne	.LBB0_3
// NO-FLAT-NEXT:    ldrb	r1, [r0]
// NO-FLAT-NEXT:    cmp	r1, #79
// NO-FLAT-NEXT:    ldrbeq	r1, [r0, #1]
// NO-FLAT-NEXT:    cmpeq	r1, #77
// NO-FLAT-NEXT:    beq	.LBB0_4

// FLAT-LABEL:    check_password:
// FLAT:            sub	sp, sp, #20
// FLAT-NEXT:       str	r1, [r11, #-48]
// FLAT-NEXT:       ldr	r1, .LCPI0_0
// FLAT-NEXT:       str	r1, [r11, #-36]
// FLAT-NEXT:       ldr	r12, .LCPI0_1
// FLAT-NEXT:       ldr	r2, .LCPI0_2
// FLAT-NEXT:       ldr	r9, .LCPI0_3
// FLAT-NEXT:       ldr	lr, .LCPI0_10
// FLAT-NEXT:       ldr	r3, .LCPI0_13
// FLAT-NEXT:       ldr	r8, .LCPI0_11
// FLAT-NEXT:       ldr	r4, .LCPI0_12
// FLAT-NEXT:       ldr	r10, .LCPI0_4
// FLAT-NEXT:       b	.LBB0_5

int check_password(const char* passwd, unsigned len) {
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

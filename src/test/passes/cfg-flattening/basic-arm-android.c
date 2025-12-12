//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: arm-registered-target && android_abi

// RUN:                                   clang -target arm-linux-android                         -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=NO-FLAT %s
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target arm-linux-android -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=FLAT-ARM %s
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target armv7-none-linux-androideabi -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=FLAT-THUMB %s

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
// FLAT-ARM:            str	r1, [sp, #4]
// FLAT-ARM-NEXT:       ldr	r1, .LCPI0_0
// FLAT-ARM-NEXT:       str	r1, [sp, #16]
// FLAT-ARM-NEXT:       ldr	r10, .LCPI0_1
// FLAT-ARM-NEXT:       ldr	r2, .LCPI0_2
// FLAT-ARM-NEXT:       ldr	r8, .LCPI0_3
// FLAT-ARM-NEXT:       ldr	lr, .LCPI0_10
// FLAT-ARM-NEXT:       ldr	r3, .LCPI0_13
// FLAT-ARM-NEXT:       ldr	r4, .LCPI0_11
// FLAT-ARM-NEXT:       ldr	r5, .LCPI0_12
// FLAT-ARM-NEXT:       ldr	r9, .LCPI0_4
// FLAT-ARM-NEXT:       b	.LBB0_4

// FLAT-THUMB:          str	r1, [sp, #4]
// FLAT-THUMB-NEXT:     movw	r1, #10559
// FLAT-THUMB-NEXT:     movt	r1, #51969
// FLAT-THUMB-NEXT:     str	r1, [sp, #16]
// FLAT-THUMB-NEXT:     movw	r10, #59329
// FLAT-THUMB-NEXT:     movt	r10, #19126
// FLAT-THUMB-NEXT:     movw	r2, #44195
// FLAT-THUMB-NEXT:     movt	r2, #62802
// FLAT-THUMB-NEXT:     movw	r8, #13808
// FLAT-THUMB-NEXT:     movt	r8, #56121
// FLAT-THUMB-NEXT:     movw	lr, #43693
// FLAT-THUMB-NEXT:     movt	lr, #47459
// FLAT-THUMB-NEXT:     movw	r3, #60166
// FLAT-THUMB-NEXT:     movt	r3, #34246
// FLAT-THUMB-NEXT:     movw	r4, #43694
// FLAT-THUMB-NEXT:     movt	r4, #47459
// FLAT-THUMB-NEXT:     movw	r5, #39329
// FLAT-THUMB-NEXT:     movt	r5, #48675
// FLAT-THUMB-NEXT:     movw	r9, #44384
// FLAT-THUMB-NEXT:     movt	r9, #62802
// FLAT-THUMB-NEXT:     b	.LBB0_4

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

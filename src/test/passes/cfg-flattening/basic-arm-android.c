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
// FLAT-ARM:            str     r1, [sp, #4]
// FLAT-ARM:            ldr     r1, .LCPI0_0
// FLAT-ARM:            str     r1, [sp, #16]
// FLAT-ARM:            ldr     lr, .LCPI0_1
// FLAT-ARM:            ldr     r2, .LCPI0_2
// FLAT-ARM:            ldr     r4, .LCPI0_3
// FLAT-ARM:            ldr     r12, .LCPI0_10
// FLAT-ARM:            ldr     r5, .LCPI0_12
// FLAT-ARM:            ldr     r3, .LCPI0_11
// FLAT-ARM:            ldr     r8, .LCPI0_4
// FLAT-ARM:            ldr     r10, .LCPI0_8
// FLAT-ARM:            b       .LBB0_2

// FLAT-THUMB:          str     r1, [sp, #4]
// FLAT-THUMB-NEXT:     movw    r1, #3951
// FLAT-THUMB-NEXT:     movt    r1, #15636
// FLAT-THUMB-NEXT:     str     r1, [sp, #16]
// FLAT-THUMB-NEXT:     movw    lr, #52532
// FLAT-THUMB-NEXT:     movt    lr, #26706
// FLAT-THUMB-NEXT:     movw    r2, #53269
// FLAT-THUMB-NEXT:     movt    r2, #45387
// FLAT-THUMB-NEXT:     movw    r4, #8047
// FLAT-THUMB-NEXT:     movt    r4, #17606
// FLAT-THUMB-NEXT:     movw    r12, #29561
// FLAT-THUMB-NEXT:     movt    r12, #50251
// FLAT-THUMB-NEXT:     movw    r5, #30275
// FLAT-THUMB-NEXT:     movt    r5, #47252
// FLAT-THUMB-NEXT:     movw    r3, #29562
// FLAT-THUMB-NEXT:     movt    r3, #50251
// FLAT-THUMB-NEXT:     movw    r8, #1332
// FLAT-THUMB-NEXT:     movt    r8, #22555
// FLAT-THUMB-NEXT:     movw    r10, #8048
// FLAT-THUMB-NEXT:     movt    r10, #17606
// FLAT-THUMB-NEXT:     b       .LBB0_2

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

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: arm-registered-target && android_abi

// RUN:                                        clang -target arm-linux-android                         -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target arm-linux-android -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R2 %s

// R0-LABEL: memcpy_xor:
// R0:       .LBB0_2:
// R0:           ldrb	r3, [r1], #1
// R0:           eor	r3, r3, #35
// R0:           strb	r3, [r12], #1
// R0:           subs	lr, lr, #1
// R0:           bne	.LBB0_2

// R2-LABEL: memcpy_xor:
// R2:       .LBB0_2:
// R2-NEXT:      ldrb    r12, [r1, lr]
// R2-NEXT:      mvn     r3, r12
// R2-NEXT:      add     r3, r12, r3, lsl #1
// R2-NEXT:      add     r3, r3, #1
// R2-NEXT:      mvn     r4, r3
// R2-NEXT:      orr     r4, r4, #35
// R2-NEXT:      add     r5, r3, r4
// R2-NEXT:      and     r4, r12, #93
// R2-NEXT:      eor     r3, r12, #221
// R2-NEXT:      add     r3, r3, r4, lsl #1
// R2-NEXT:      add     r3, r3, r5, lsl #1
// R2-NEXT:      add     r3, r3, #2
// R2-NEXT:      strb    r3, [r0, lr]
// R2-NEXT:      and     r3, lr, #2
// R2-NEXT:      eor     r5, lr, #2
// R2-NEXT:      add     r3, r5, r3, lsl #1
// R2-NEXT:      mvn     r5, r3
// R2-NEXT:      add     lr, r5, r3, lsl #1
// R2-NEXT:      cmp     lr, r2
// R2-NEXT:      blo     .LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

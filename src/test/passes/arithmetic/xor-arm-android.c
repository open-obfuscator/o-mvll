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
// R2-NEXT:      ldrb	r4, [r1, r3]
// R2-NEXT:      bic	r5, r12, r4, lsl #1
// R2-NEXT:      add	r4, r4, r5
// R2-NEXT:      sub	r4, r4, #35
// R2-NEXT:      strb	r4, [r0, r3]
// R2-NEXT:      mvn	r4, r3
// R2-NEXT:      orr	r4, r4, lr
// R2-NEXT:      add	r4, r3, r4
// R2-NEXT:      and	r3, r3, #1
// R2-NEXT:      add	r3, r4, r3
// R2-NEXT:      add	r3, r3, #2
// R2-NEXT:      cmp	r3, r2
// R2-NEXT:      blo	.LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

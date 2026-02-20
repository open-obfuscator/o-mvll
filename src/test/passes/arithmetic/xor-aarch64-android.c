//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: aarch64-registered-target && android_abi

// RUN:                                        clang -target aarch64-linux-android                         -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_0.py clang -target aarch64-linux-android -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_1.py clang -target aarch64-linux-android -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R1 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target aarch64-linux-android -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R2 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_3.py clang -target aarch64-linux-android -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R3 %s

// R0-LABEL: memcpy_xor:
// R0:       .LBB0_2:
// R0:           ldrb	w11, [x1], #1
// R0:           subs	x8, x8, #1
// R0:           eor	w11, w11, w9
// R0:           strb	w11, [x10], #1
// R0:           b.ne	.LBB0_2

// R1-LABEL: memcpy_xor:
// R1:       .LBB0_2:
// R1-NEXT:      ldrb	w11, [x1], #1
// R1-NEXT:      subs	x8, x8, #1
// R1-NEXT:      bic	w12, w9, w11, lsl #1
// R1-NEXT:      add	w11, w11, w12
// R1-NEXT:      sub	w11, w11, #35
// R1-NEXT:      strb	w11, [x10], #1
// R1-NEXT:      b.ne	.LBB0_2

// R2-LABEL: memcpy_xor:
// R2:       .LBB0_2:
// R2-NEXT:      mov	w10, w8
// R2-NEXT:      and	w12, w8, #0x1
// R2-NEXT:      mvn	w13, w8
// R2-NEXT:      add	w8, w8, w12
// R2-NEXT:      orr	w13, w13, #0xfffffffe
// R2-NEXT:      ldrb	w11, [x1, x10]
// R2-NEXT:      add	w8, w8, w13
// R2-NEXT:      add	w8, w8, #2
// R2-NEXT:      cmp	w8, w2
// R2-NEXT:      bic	w14, w9, w11, lsl #1
// R2-NEXT:      add	w11, w11, w14
// R2-NEXT:      sub	w11, w11, #35
// R2-NEXT:      strb	w11, [x0, x10]
// R2-NEXT:      b.lo	.LBB0_2

// R3-LABEL: memcpy_xor:
// R3:       .LBB0_2:
// R3-NEXT:      ldrb	w11, [x1], #1
// R3-NEXT:      subs	x8, x8, #1
// R3-NEXT:      and	w12, w11, #0x1
// R3-NEXT:      orr	w13, w11, #0x1
// R3-NEXT:      add	w12, w13, w12
// R3-NEXT:      bic	w11, w9, w11, lsl #1
// R3-NEXT:      add	w11, w12, w11
// R3-NEXT:      sub	w11, w11, #36
// R3-NEXT:      strb	w11, [x10], #1
// R3-NEXT:      b.ne	.LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

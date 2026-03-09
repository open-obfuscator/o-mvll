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
// R2-NEXT:      mov     w10, w9
// R2-NEXT:      lsl     w13, w9, #1
// R2-NEXT:      eor     w9, w9, #0x2
// R2-NEXT:      and     w13, w13, #0x4
// R2-NEXT:      add     w9, w13, w9
// R2-NEXT:      ldrb    w11, [x1, x10]
// R2-NEXT:      mvn     w12, w11
// R2-NEXT:      add     w12, w11, w12, lsl #1
// R2-NEXT:      add     w12, w12, #1
// R2-NEXT:      orn     w14, w8, w12
// R2-NEXT:      add     w12, w12, w14
// R2-NEXT:      lsl     w14, w11, #1
// R2-NEXT:      eor     w11, w11, #0xdddddddd
// R2-NEXT:      and     w14, w14, #0xbbbbbbbb
// R2-NEXT:      add     w11, w14, w11
// R2-NEXT:      add     w11, w11, w12, lsl #1
// R2-NEXT:      mvn     w12, w9
// R2-NEXT:      add     w9, w12, w9, lsl #1
// R2-NEXT:      add     w11, w11, #2
// R2-NEXT:      cmp     w9, w2
// R2-NEXT:      strb    w11, [x0, x10]
// R2-NEXT:      b.lo    .LBB0_2

// R3-LABEL: memcpy_xor:
// R3:       .LBB0_2:
// R3-NEXT:      mov     w12, w8
// R3-NEXT:      ldrb    w13, [x1, x12]
// R3-NEXT:      mvn     w14, w13
// R3-NEXT:      lsl     w16, w13, #1
// R3-NEXT:      eor     w15, w13, #0x1
// R3-NEXT:      and     w17, w16, #0x2
// R3-NEXT:      add     w14, w13, w14, lsl #1
// R3-NEXT:      add     w15, w15, w17
// R3-NEXT:      bic     w16, w10, w16
// R3-NEXT:      add     w14, w15, w14, lsl #1
// R3-NEXT:      add     w14, w14, #2
// R3-NEXT:      mvn     w15, w14
// R3-NEXT:      add     w14, w14, w15, lsl #1
// R3-NEXT:      add     w15, w14, #1
// R3-NEXT:      orn     w17, w9, w15
// R3-NEXT:      add     w15, w15, w17
// R3-NEXT:      orn     w17, w11, w13
// R3-NEXT:      add     w15, w15, #36
// R3-NEXT:      add     w17, w13, w17
// R3-NEXT:      eon     w3, w14, w15
// R3-NEXT:      bic     w14, w15, w14
// R3-NEXT:      lsl     w15, w8, #1
// R3-NEXT:      add     w17, w17, w3
// R3-NEXT:      and     w15, w15, #0x4
// R3-NEXT:      add     w14, w17, w14, lsl #1
// R3-NEXT:      add     w17, w8, w15
// R3-NEXT:      eor     w15, w15, #0x4
// R3-NEXT:      add     w15, w17, w15
// R3-NEXT:      add     w13, w13, w16
// R3-NEXT:      sub     w15, w15, #2
// R3-NEXT:      add     w13, w13, w14, lsl #1
// R3-NEXT:      orr     w14, w15, #0x1
// R3-NEXT:      orr     w8, w8, #0xfffffffe
// R3-NEXT:      add     w8, w8, w14
// R3-NEXT:      add     w13, w13, #37
// R3-NEXT:      cmp     w8, w2
// R3-NEXT:      strb    w13, [x0, x12]
// R3-NEXT:      b.lo    .LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

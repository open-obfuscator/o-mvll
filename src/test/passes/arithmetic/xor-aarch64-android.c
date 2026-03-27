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
// R1-NEXT:   	ldrb	w11, [x1], #1
// R1-NEXT:   	subs	x8, x8, #1
// R1-NEXT:   	eor	w11, w11, w9
// R1-NEXT:   	strb	w11, [x10], #1
// R1-NEXT:   	b.ne	.LBB0_2

// R2-LABEL: memcpy_xor:
// R2:       .LBB0_2:
// R2-NEXT:      ldrb	w12, [x1], #1
// R2-NEXT:      subs	x8, x8, #1
// R2-NEXT:      orn	w13, w9, w12
// R2-NEXT:      add	w13, w12, w13
// R2-NEXT:      and	w12, w12, w10
// R2-NEXT:      add	w13, w13, #36
// R2-NEXT:      neg	w12, w12
// R2-NEXT:      and	w14, w13, w12
// R2-NEXT:      eor	w12, w13, w12
// R2-NEXT:      add	w12, w12, w14, lsl #1
// R2-NEXT:      strb	w12, [x11], #1
// R2-NEXT:      b.ne	.LBB0_2


// R3-LABEL: memcpy_xor:
// R3:       .LBB0_2:
// R3-NEXT:      mov	w11, w9
// R3-NEXT:      ldrb	w12, [x1, x11]
// R3-NEXT:      mvn	w13, w12
// R3-NEXT:      orr	w14, w12, w10
// R3-NEXT:      add	w15, w12, #35
// R3-NEXT:      neg	w14, w14
// R3-NEXT:      add	w13, w12, w13, lsl #1
// R3-NEXT:      and	w16, w15, w14
// R3-NEXT:      eor	w14, w15, w14
// R3-NEXT:      add	w13, w13, #1
// R3-NEXT:      orr	w13, w13, w8
// R3-NEXT:      add	w14, w14, w16, lsl #1
// R3-NEXT:      lsl	w15, w9, #1
// R3-NEXT:      add	w12, w12, w13
// R3-NEXT:      neg	w13, w14
// R3-NEXT:      and	w14, w15, #0x4
// R3-NEXT:      add	w12, w12, #36
// R3-NEXT:      add	w9, w9, w14
// R3-NEXT:      eor	w14, w14, #0x4
// R3-NEXT:      and	w15, w12, w13
// R3-NEXT:      add	w9, w9, w14
// R3-NEXT:      eor	w12, w12, w13
// R3-NEXT:      sub	w9, w9, #3
// R3-NEXT:      add	w12, w12, w15, lsl #1
// R3-NEXT:      cmp	w9, w2
// R3-NEXT:      strb	w12, [x0, x11]
// R3-NEXT:      b.lo	.LBB0_2


void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

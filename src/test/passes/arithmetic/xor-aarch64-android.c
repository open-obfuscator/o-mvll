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
// R2-NEXT:     mov	w11, w9
// R2-NEXT: 	orr	w13, w9, #0x1
// R2-NEXT: 	ldrb	w12, [x1, x11]
// R2-NEXT: 	orn	w14, w8, w12
// R2-NEXT: 	and	w15, w12, w10
// R2-NEXT: 	add	w12, w12, w14
// R2-NEXT: 	neg	w14, w15
// R2-NEXT: 	mvn	w15, w9
// R2-NEXT: 	add	w12, w12, #36
// R2-NEXT: 	add	w9, w9, w13
// R2-NEXT: 	orr	w15, w15, #0x1
// R2-NEXT: 	and	w13, w12, w14
// R2-NEXT: 	add	w9, w9, w15
// R2-NEXT: 	eor	w12, w12, w14
// R2-NEXT: 	add	w9, w9, #1
// R2-NEXT: 	add	w12, w12, w13, lsl #1
// R2-NEXT: 	cmp	w9, w2
// R2-NEXT: 	strb	w12, [x0, x11]
// R2-NEXT: 	b.lo	.LBB0_2

// R3-LABEL: memcpy_xor:
// R3:       .LBB0_2:
// R3-NEXT:     mov	w15, w9
// R3-NEXT: 	sub	w16, w13, w9
// R3-NEXT: 	orr	w3, w9, #0xfffffffe
// R3-NEXT: 	lsl	w4, w9, #1
// R3-NEXT: 	add	w3, w16, w3
// R3-NEXT: 	add	w16, w16, w4
// R3-NEXT: 	ldrb	w17, [x1, x15]
// R3-NEXT: 	eor	w5, w16, w3
// R3-NEXT: 	and	w16, w16, w3
// R3-NEXT: 	bic	w4, w14, w4
// R3-NEXT: 	eor	w3, w17, w8
// R3-NEXT: 	and	w6, w17, w10
// R3-NEXT: 	eor	w6, w6, w11
// R3-NEXT: 	add	w3, w3, w17
// R3-NEXT: 	and	w17, w17, w8
// R3-NEXT: 	add	w3, w3, w6
// R3-NEXT: 	neg	w6, w17
// R3-NEXT: 	sub	w7, w12, w3
// R3-NEXT: 	and	w19, w7, w6
// R3-NEXT: 	orr	w6, w7, w6
// R3-NEXT: 	add	w17, w17, w3
// R3-NEXT: 	add	w3, w3, w6
// R3-NEXT: 	mvn	w6, w9
// R3-NEXT: 	add	w9, w16, w9
// R3-NEXT: 	orr	w6, w6, #0x1
// R3-NEXT: 	add	w17, w17, w19, lsl #1
// R3-NEXT: 	add	w16, w4, w6
// R3-NEXT: 	add	w17, w17, w3, lsl #1
// R3-NEXT: 	add	w16, w16, w5
// R3-NEXT: 	add	w17, w17, #110
// R3-NEXT: 	add	w9, w16, w9, lsl #1
// R3-NEXT: 	cmp	w9, w2
// R3-NEXT: 	strb	w17, [x0, x15]
// R3-NEXT: 	b.lo	.LBB0_2


void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: aarch64-registered-target && apple_abi

// RUN:                                        clang -target arm64-apple-ios                         -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_0.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_1.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R1 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R2 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_3.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R3 %s

// R0-LABEL: _memcpy_xor:
// R0:       LBB0_2:
// R0-NEXT:      ldrb    w11, [x1], #1
// R0-NEXT:      eor     w11, w11, w9
// R0-NEXT:      strb    w11, [x10], #1
// R0-NEXT:      subs    x8, x8, #1
// R0-NEXT:      b.ne    LBB0_2

// R1-LABEL: _memcpy_xor:
// R1:       LBB0_2:
// R1-NEXT:      ldrb    w11, [x1], #1
// R1-NEXT:      bic     w12, w9, w11, lsl #1
// R1-NEXT:      add     w11, w11, w12
// R1-NEXT:      sub     w11, w11, #35
// R1-NEXT:      strb    w11, [x10], #1
// R1-NEXT:      subs    x8, x8, #1
// R1-NEXT:      b.ne    LBB0_2

// R2-LABEL: _memcpy_xor:
// R2:       LBB0_2:
// R2-NEXT:      ldrsb   w11, [x1], #1
// R2-NEXT:      mvn     w12, w11
// R2-NEXT:      add     w12, w11, w12, lsl #1
// R2-NEXT:      add     w12, w12, #1
// R2-NEXT:      orn     w13, w9, w12
// R2-NEXT:      add     w12, w12, w13
// R2-NEXT:      eor     w13, w11, #0xdddddddd
// R2-NEXT:      lsl     w11, w11, #1
// R2-NEXT:      and     w11, w11, #0xbbbbbbbb
// R2-NEXT:      add     w11, w11, w13
// R2-NEXT:      add     w11, w11, w12, lsl #1
// R2-NEXT:      add     w11, w11, #2
// R2-NEXT:      strb    w11, [x10], #1
// R2-NEXT:      subs    x8, x8, #1
// R2-NEXT:      b.ne    LBB0_2

// R3-LABEL: _memcpy_xor:
// R3:       LBB0_2:
// R3-NEXT:      ldrsb   w12, [x1, w8, uxtw]
// R3-NEXT:      mvn     w13, w12
// R3-NEXT:      add     w13, w12, w13, lsl #1
// R3-NEXT:      add     w13, w13, #1
// R3-NEXT:      mvn     w14, w13
// R3-NEXT:      add     w14, w13, w14, lsl #1
// R3-NEXT:      add     w14, w14, #1
// R3-NEXT:      orn     w15, w9, w14
// R3-NEXT:      add     w13, w13, w14
// R3-NEXT:      add     w13, w13, w15
// R3-NEXT:      lsl     w13, w13, #1
// R3-NEXT:      and     w14, w10, w12, lsl #1
// R3-NEXT:      eor     w15, w14, w10
// R3-NEXT:      add     w12, w12, w14
// R3-NEXT:      add     w12, w12, w15
// R3-NEXT:      add     w14, w12, #35
// R3-NEXT:      add     w13, w13, #74
// R3-NEXT:      orn     w15, w13, w14
// R3-NEXT:      add     w12, w12, w15
// R3-NEXT:      orr     w13, w13, w14
// R3-NEXT:      add     w12, w12, w13
// R3-NEXT:      add     w12, w12, #36
// R3-NEXT:      strb    w12, [x0, w8, uxtw]
// R3-NEXT:      bic     w12, w11, w8, lsl #1
// R3-NEXT:      add     w12, w8, w12
// R3-NEXT:      mvn     w13, w8
// R3-NEXT:      orr     w13, w13, #0x2
// R3-NEXT:      add     w13, w8, w13
// R3-NEXT:      add     w12, w12, w13, lsl #1
// R3-NEXT:      and     w12, w12, #0xfffffffe
// R3-NEXT:      orr     w8, w8, #0xfffffffe
// R3-NEXT:      add     w8, w8, w12
// R3-NEXT:      add     w8, w8, #1
// R3-NEXT:      cmp     w8, w2
// R3-NEXT:      b.lo    LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

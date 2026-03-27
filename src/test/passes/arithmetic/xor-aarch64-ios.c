//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: aarch64-registered-target && apple_abi

// RUN:                                        clang -target arm64-apple-ios                         -O0 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_0.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O0 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_1.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O0 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R1 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O0 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R2 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_3.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O0 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R3 %s

// R0-LABEL: _memcpy_xor:
// R0:       LBB0_2:
// R0-NEXT:      ldr     x8, [sp, #16]
// R0-NEXT:      ldr     w9, [sp, #8]
// R0-NEXT:      ldrsb   w8, [x8, x9]
// R0-NEXT:      mov     w9, #35
// R0-NEXT:      eor     w8, w8, w9
// R0-NEXT:      ldr     x9, [sp, #24]
// R0-NEXT:      ldr     w10, [sp, #8]
// R0-NEXT:      strb    w8, [x9, x10]
// R0-NEXT:      b       LBB0_3

// R1-LABEL: _memcpy_xor:
// R1:       LBB0_2:
// R1-NEXT:      ldr     x8, [sp, #16]
// R1-NEXT:      ldr     w9, [sp, #8]
// R1-NEXT:      ldrsb   w9, [x8, x9]
// R1-NEXT:      mov     w10, #35
// R1-NEXT:      orr     w8, w9, w10
// R1-NEXT:      and     w9, w9, w10
// R1-NEXT:      subs    w8, w8, w9
// R1-NEXT:      ldr     x9, [sp, #24]
// R1-NEXT:      ldr     w10, [sp, #8]
// R1-NEXT:      strb    w8, [x9, x10]
// R1-NEXT:      b       LBB0_3

// R2-LABEL: _memcpy_xor:
// R2:       LBB0_2:
// R2-NEXT:      ldr     x8, [sp, #16]
// R2-NEXT:      ldr     w9, [sp, #8]
// R2-NEXT:      ldrsb   w10, [x8, x9]
// R2-NEXT:      mov     w11, #35
// R2-NEXT:      add     w8, w10, #35
// R2-NEXT:      add     w8, w8, #1
// R2-NEXT:      mov     w9, #-36
// R2-NEXT:      orn     w9, w9, w10
// R2-NEXT:      add     w9, w8, w9
// R2-NEXT:      add     w8, w10, #35
// R2-NEXT:      orr     w10, w10, w11
// R2-NEXT:      subs    w11, w8, w10
// R2-NEXT:      mov     w10, #0
// R2-NEXT:      subs    w8, w10, w11
// R2-NEXT:      eor     w8, w9, w8
// R2-NEXT:      subs    w10, w10, w11
// R2-NEXT:      and     w9, w9, w10
// R2-NEXT:      add     w8, w8, w9, lsl #1
// R2-NEXT:      ldr     x9, [sp, #24]
// R2-NEXT:      ldr     w10, [sp, #8]
// R2-NEXT:      strb    w8, [x9, x10]
// R2-NEXT:      b       LBB0_3

// R3-LABEL: _memcpy_xor:
// R3:       LBB0_2:
// R3-NEXT:      ldr     x8, [sp, #16]
// R3-NEXT:      ldr     w9, [sp, #8]
// R3-NEXT:      ldrsb   w12, [x8, x9]
// R3-NEXT:      add     w8, w12, #36
// R3-NEXT:      subs    w9, w8, #1
// R3-NEXT:      and     w8, w9, #0x1
// R3-NEXT:      orr     w9, w9, #0x1
// R3-NEXT:      add     w10, w8, w9
// R3-NEXT:      mov     w9, #-1
// R3-NEXT:      mvn     w11, w12
// R3-NEXT:      add     w8, w12, #1
// R3-NEXT:      add     w8, w8, w11, lsl #1
// R3-NEXT:      mov     w13, #35
// R3-NEXT:      and     w8, w8, w13
// R3-NEXT:      subs    w11, w8, #36
// R3-NEXT:      and     w8, w10, w11
// R3-NEXT:      orr     w10, w10, w11
// R3-NEXT:      add     w11, w8, w10
// R3-NEXT:      add     w8, w12, #36
// R3-NEXT:      subs    w10, w8, #1
// R3-NEXT:      eor     w8, w12, w13
// R3-NEXT:      and     w12, w12, w13
// R3-NEXT:      add     w13, w8, w12
// R3-NEXT:      mov     w12, #0
// R3-NEXT:      subs    w8, w12, w13
// R3-NEXT:      eor     w8, w10, w8
// R3-NEXT:      subs    w12, w12, w13
// R3-NEXT:      and     w10, w10, w12
// R3-NEXT:      add     w10, w8, w10, lsl #1
// R3-NEXT:      mvn     w8, w10
// R3-NEXT:      add     w12, w8, #1
// R3-NEXT:      orr     w8, w11, w12
// R3-NEXT:      and     w12, w11, w12
// R3-NEXT:      subs    w8, w8, w12
// R3-NEXT:      mvn     w10, w10
// R3-NEXT:      add     w12, w10, #1
// R3-NEXT:      add     w10, w11, w12
// R3-NEXT:      orr     w11, w11, w12
// R3-NEXT:      subs    w10, w10, w11
// R3-NEXT:      eor     w9, w9, w10, lsl #1
// R3-NEXT:      subs    w8, w8, w9
// R3-NEXT:      subs    w8, w8, #1
// R3-NEXT:      ldr     x9, [sp, #24]
// R3-NEXT:      ldr     w10, [sp, #8]
// R3-NEXT:      strb    w8, [x9, x10]
// R3-NEXT:      b       LBB0_3

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

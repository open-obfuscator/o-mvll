// REQUIRES: aarch64-registered-target

// RUN:                                        clang -target arm64-apple-ios                         -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_0.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_1.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R1 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R2 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_3.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=R3 %s

// R0-LABEL: _memcpy_xor:
// R0:       LBB0_2:
// R0:           ldrb	w11, [x1], #1
// R0:           eor	w11, w11, w9
// R0:           strb	w11, [x10], #1
// R0:           subs	x8, x8, #1
// R0:           b.ne	LBB0_2

// FIXME: It seems like rounds=1 doesn't actually do anything!
// R1-LABEL: _memcpy_xor:
// R1:       LBB0_2:
// R1:           ldrb	w11, [x1], #1
// R1:           eor	w11, w11, w9
// R1:           strb	w11, [x10], #1
// R1:           subs	x8, x8, #1
// R1:           b.ne	LBB0_2

// R2-LABEL: _memcpy_xor:
// R2:       LBB0_2:
// R2:       	mov	w11, w8
// R2:       	ldrsb	w12, [x1, x11]
// R2:       	orn	w13, w9, w12
// R2:       	add	w13, w12, w13
// R2:       	add	w13, w13, #36
// R2:       	and	w12, w12, w10
// R2:       	neg	w12, w12
// R2:       	eor	w14, w13, w12
// R2:       	and	w12, w13, w12
// R2:       	add	w12, w14, w12, lsl #1
// R2:       	strb	w12, [x0, x11]
// R2:       	and	w11, w8, #0x1
// R2:       	mvn	w12, w8
// R2:       	orr	w12, w12, #0xfffffffe
// R2-DAG:      add	w8, w8, w12
// R2-DAG:      add	w8, w8, w11
// R2:       	add	w8, w8, #2
// R2:       	cmp	w8, w2
// R2:       	b.lo	LBB0_2

// R3-LABEL: _memcpy_xor:
// R3:       LBB0_2:
// R3:       	mov	w12, w8
// R3:       	ldrb	w13, [x1, x12]
// R3:       	add	w14, w13, #35
// R3:       	orr	w15, w13, w9
// R3:       	orn	w16, w10, w13
// R3:       	sub	w13, w10, w13
// R3:       	sub	w13, w13, w16
// R3:       	eor	w16, w13, w14
// R3:       	and	w13, w13, w14
// R3:       	add	w13, w16, w13, lsl #1
// R3:       	neg	w13, w13
// R3:       	eor	w14, w13, w15
// R3:       	and	w13, w13, w15
// R3:       	add	w13, w14, w13, lsl #1
// R3:       	strb	w13, [x0, x12]
// R3:       	add	w12, w8, #1
// R3:       	mvn	w13, w8
// R3:       	orr	w13, w13, #0xfffffffe
// R3:       	sub	w14, w11, w8
// R3:       	sub	w13, w14, w13
// R3:       	eor	w14, w13, w12
// R3:       	and	w12, w13, w12
// R3:       	orr	w8, w8, #0x1
// R3:       	add	w8, w14, w8
// R3:       	add	w8, w8, w12, lsl #1
// R3:       	cmp	w8, w2
// R3:       	b.lo	LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

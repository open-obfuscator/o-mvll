//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: aarch64-registered-target && apple_abi

// RUN:                                   clang -target arm64-apple-ios                         -O1 -fno-verbose-asm -S %s -o - | FileCheck %s
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target arm64-apple-ios -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=BREAKCFG-IOS %s

// BREAKCFG-IOS-LABEL: _compute.1:
// BREAKCFG-IOS:        	.cfi_startproc
// BREAKCFG-IOS-NEXT:   	.byte	16
// BREAKCFG-IOS-NEXT:   	.byte	0
// BREAKCFG-IOS-NEXT:   	.byte	0
// BREAKCFG-IOS-NEXT:   	.byte	16
// BREAKCFG-IOS-NEXT:   	.byte	32
// BREAKCFG-IOS-NEXT:   	.byte	12
// BREAKCFG-IOS-NEXT:   	.byte	64
// BREAKCFG-IOS-NEXT:   	.byte	249
// BREAKCFG-IOS-NEXT:   	.byte	65
// BREAKCFG-IOS-NEXT:   	.byte	0
// BREAKCFG-IOS-NEXT:   	.byte	0
// BREAKCFG-IOS-NEXT:   	.byte	88
// BREAKCFG-IOS-NEXT:   	.byte	32
// BREAKCFG-IOS-NEXT:   	.byte	2
// BREAKCFG-IOS-NEXT:   	.byte	63
// BREAKCFG-IOS-NEXT:   	.byte	214
// BREAKCFG-IOS-NEXT:   	.byte	65
// BREAKCFG-IOS-NEXT:   	.byte	0
// BREAKCFG-IOS-NEXT:   	.byte	0
// BREAKCFG-IOS-NEXT:   	.byte	88
// BREAKCFG-IOS-NEXT:   	.byte	96
// BREAKCFG-IOS-NEXT:   	.byte	6
// BREAKCFG-IOS-NEXT:   	.byte	63
// BREAKCFG-IOS-NEXT:   	.byte	214
// BREAKCFG-IOS-NEXT:   	.byte	241
// BREAKCFG-IOS-NEXT:   	.byte	255
// BREAKCFG-IOS-NEXT:   	.byte	242
// BREAKCFG-IOS-NEXT:   	.byte	162
// BREAKCFG-IOS-NEXT:   	.byte	248
// BREAKCFG-IOS-NEXT:   	.byte	255
// BREAKCFG-IOS-NEXT:   	.byte	226
// BREAKCFG-IOS-NEXT:   	.byte	194
// BREAKCFG-IOS-NEXT:   	add	w8, w1, w0
// BREAKCFG-IOS-NEXT:   	madd	w0, w1, w0, w8
// BREAKCFG-IOS-NEXT:   	ret

// CHECK-LABEL:         _compute:
// BREAKCFG-IOS:        Lloh0:
// BREAKCFG-IOS-NEXT:   	adrp	x8, _compute.1@PAGE
// BREAKCFG-IOS-NEXT:   Lloh1:
// BREAKCFG-IOS-NEXT:   	add	x8, x8, _compute.1@PAGEOFF
// BREAKCFG-IOS-NEXT:   	str	x8, [sp, #-16]!
// BREAKCFG-IOS-NEXT:   	.cfi_def_cfa_offset 16
// BREAKCFG-IOS-NEXT:   	add	x8, x8, #32
// BREAKCFG-IOS-NEXT:   	str	x8, [sp, #8]
// BREAKCFG-IOS-NEXT:   	ldr	x2, [sp, #8]
// BREAKCFG-IOS-NEXT:   	add	sp, sp, #16
// BREAKCFG-IOS-NEXT:   	br	x2

inline int add(int a, int b) {
    return a + b;
}

__attribute__((always_inline)) inline int multiply(int a, int b) {
    return a * b;
}

int compute(int a, int b) {
    int sum     = add(a, b);
    int product = multiply(a, b);
    return sum + product;
}

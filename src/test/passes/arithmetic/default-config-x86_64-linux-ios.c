//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: x86-registered-target && apple_abi

// RUN:                                                              clang -target x86_64-pc-linux-gnu                         -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_empty_params.py     clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_include_function.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=CHECK %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_exclude_module.py   clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_exclude_function.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_probability_0.py    clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_probability_100.py  clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -fno-verbose-asm -O1 -S %s -o - | FileCheck --check-prefix=CHECKV2 %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_include_seed.py     clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -fno-verbose-asm -O1 -S %s -o - | FileCheck --check-prefix=SEED %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_include_last.py     clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -fno-verbose-asm -O1 -S %s -o - | FileCheck --check-prefix=SEEDV2 %s

// DEFAULT-LABEL: memcpy_xor:
// DEFAULT:       .LBB0_2:
// DEFAULT:           movzbl	(%rsi,%rcx), %edx
// DEFAULT:           xorb	$35, %dl
// DEFAULT:           movb	%dl, (%rdi,%rcx)
// DEFAULT:           incq	%rcx
// DEFAULT:           cmpq	%rcx, %rax
// DEFAULT:           jne	.LBB0_2

// CHECK-LABEL: memcpy_xor:
// CHECK:       .LBB0_2:
// CHECK-NEXT:     movsbl	(%rsi,%rcx), %edx
// CHECK-NEXT:     movl	%edx, %r8d
// CHECK-NEXT:     notl	%r8d
// CHECK-NEXT:     orl	$-36, %r8d
// CHECK-NEXT:     addl	%edx, %r8d
// CHECK-NEXT:     addl	$36, %r8d
// CHECK-NEXT:     andl	$35, %edx
// CHECK-NEXT:     negl	%edx
// CHECK-NEXT:     movl	%r8d, %r9d
// CHECK-NEXT:     xorl	%edx, %r9d
// CHECK-NEXT:     andl	%r8d, %edx
// CHECK-NEXT:     leal	(%r9,%rdx,2), %edx
// CHECK-NEXT:     movb	%dl, (%rdi,%rcx)
// CHECK-NEXT:     incq	%rcx
// CHECK-NEXT:     cmpq	%rcx, %rax
// CHECK-NEXT:     jne	.LBB0_2

// This is the exact same config as CHECK (include_function) but with
// a different probability seed, thus the instruction changes
// 
// First variant: Optimizer is undoing Arithmetics
// 
// SEED-LABEL: memcpy_xor:
// SEED:     .LBB0_2:
// SEED-NEXT:    movzbl	(%rsi,%rcx), %edx
// SEED-NEXT:    xorb	$35, %dl
// SEED-NEXT:    movb	%dl, (%rdi,%rcx)
// SEED-NEXT:    incq	%rcx
// SEED-NEXT:    cmpq	%rcx, %rax
// SEED-NEXT:    jne	.LBB0_2

// Second variant with Arithmetic in Phase.Last
// Arithmetic runs after Optimizer
// 
// SEEDV2-LABEL: memcpy_xor:
// SEEDV2:       .LBB0_2: 
// SEEDV2-NEXT:     movzbl	(%rsi,%rcx), %edx
// SEEDV2-NEXT:   	movl	%edx, %r8d
// SEEDV2-NEXT:   	orb	$35, %r8b
// SEEDV2-NEXT:   	subb	%dl, %r8b
// SEEDV2-NEXT:   	movl	%edx, %r9d
// SEEDV2-NEXT:   	xorb	$-35, %r9b
// SEEDV2-NEXT:   	andb	$93, %dl
// SEEDV2-NEXT:   	addb	%dl, %dl
// SEEDV2-NEXT:   	addb	%r9b, %dl
// SEEDV2-NEXT:   	addb	%r8b, %r8b
// SEEDV2-NEXT:   	addb	%dl, %r8b
// SEEDV2-NEXT:   	movb	%r8b, (%rdi,%rcx)
// SEEDV2-NEXT:   	movq	%rcx, %rdx
// SEEDV2-NEXT:   	orq	$1, %rdx
// SEEDV2-NEXT:   	movq	%rcx, %r8
// SEEDV2-NEXT:   	subq	%rdx, %r8
// SEEDV2-NEXT:   	movq	%rcx, %rdx
// SEEDV2-NEXT:   	xorq	$1, %rdx
// SEEDV2-NEXT:   	andl	$1, %ecx
// SEEDV2-NEXT:   	addq	%rdx, %rcx
// SEEDV2-NEXT:   	addq	%r8, %rcx
// SEEDV2-NEXT:   	incq	%rcx
// SEEDV2-NEXT:   	cmpq	%rax, %rcx
// SEEDV2-NEXT:   	jne	.LBB0_2

// CHECKV2-LABEL: memcpy_xor:
// CHECKV2:     .LBB0_2:
// CHECKV2-NEXT:    movl	%eax, %ecx
// CHECKV2-NEXT:   	movsbl	(%rsi,%rcx), %r8d
// CHECKV2-NEXT:   	movl	%r8d, %r9d
// CHECKV2-NEXT:   	notl	%r9d
// CHECKV2-NEXT:   	orl	$-36, %r9d
// CHECKV2-NEXT:   	addl	%r8d, %r9d
// CHECKV2-NEXT:   	addl	$36, %r9d
// CHECKV2-NEXT:   	andl	$35, %r8d
// CHECKV2-NEXT:   	negl	%r8d
// CHECKV2-NEXT:   	movl	%r9d, %r10d
// CHECKV2-NEXT:   	xorl	%r8d, %r10d
// CHECKV2-NEXT:   	andl	%r9d, %r8d
// CHECKV2-NEXT:   	leal	(%r10,%r8,2), %r8d
// CHECKV2-NEXT:   	movb	%r8b, (%rdi,%rcx)
// CHECKV2-NEXT:   	movl	%eax, %ecx
// CHECKV2-NEXT:   	notl	%ecx
// CHECKV2-NEXT:   	orl	$1, %ecx
// CHECKV2-NEXT:   	addl	%eax, %ecx
// CHECKV2-NEXT:   	orl	$1, %eax
// CHECKV2-NEXT:   	addl	%ecx, %eax
// CHECKV2-NEXT:   	incl	%eax
// CHECKV2-NEXT:   	cmpl	%edx, %eax
// CHECKV2-NEXT:   	jb	.LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

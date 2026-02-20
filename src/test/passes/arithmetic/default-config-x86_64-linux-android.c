//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: x86-registered-target && android_abi

// RUN:                                                                  clang -target x86_64-pc-linux-gnu                         -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_empty_params.py         clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_include_function.py     clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -fno-verbose-asm -O1 -S %s -o - | FileCheck --check-prefix=CHECK %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_exclude_module.py       clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_exclude_function.py     clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_probability_0.py        clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_probability_100.py      clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -fno-verbose-asm -O1 -S %s -o - | FileCheck --check-prefix=CHECKV2 %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_include_new_seed.py     clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -fno-verbose-asm -O1 -S %s -o - | FileCheck --check-prefix=NEWSEED %s

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
// CHECK-NEXT:      movl	%eax, %ecx
// CHECK-NEXT:      movzbl	(%rsi,%rcx), %r8d
// CHECK-NEXT:      movl	%r8d, %r9d
// CHECK-NEXT:      notb	%r9b
// CHECK-NEXT:      addb	%r9b, %r9b
// CHECK-NEXT:      andb	$70, %r9b
// CHECK-NEXT:      addb	%r8b, %r9b
// CHECK-NEXT:      addb	$-35, %r9b
// CHECK-NEXT:      movb	%r9b, (%rdi,%rcx)
// CHECK-NEXT:      movl	%eax, %ecx
// CHECK-NEXT:      notl	%ecx
// CHECK-NEXT:      orl	$-2, %ecx
// CHECK-NEXT:      addl	%eax, %ecx
// CHECK-NEXT:      andl	$1, %eax
// CHECK-NEXT:      addl	%ecx, %eax
// CHECK-NEXT:      addl	$2, %eax
// CHECK-NEXT:      cmpl	%edx, %eax
// CHECK-NEXT:      jb	.LBB0_2

// This is the exact same config as CHECK (include_function) but with
// a different probability seed, thus the instruction changes
// NEWSEED-LABEL: memcpy_xor:
// NEWSEED:     .LBB0_2:
// NEWSEED-NEXT:  	movl	%eax, %ecx
// NEWSEED-NEXT:  	movsbl	(%rsi,%rcx), %r8d
// NEWSEED-NEXT:  	movl	%r8d, %r9d
// NEWSEED-NEXT:  	orl	$35, %r9d
// NEWSEED-NEXT:  	notl	%r8d
// NEWSEED-NEXT:  	movl	%r8d, %r10d
// NEWSEED-NEXT:  	orl	$35, %r10d
// NEWSEED-NEXT:  	subl	%r10d, %r8d
// NEWSEED-NEXT:  	movl	%r8d, %r10d
// NEWSEED-NEXT:  	xorl	%r9d, %r10d
// NEWSEED-NEXT:  	andl	%r9d, %r8d
// NEWSEED-NEXT:  	leal	(%r10,%r8,2), %r8d
// NEWSEED-NEXT:  	movb	%r8b, (%rdi,%rcx)
// NEWSEED-NEXT:  	movl	%eax, %ecx
// NEWSEED-NEXT:  	xorl	$2, %ecx
// NEWSEED-NEXT:  	andl	$2, %eax
// NEWSEED-NEXT:  	leal	(%rcx,%rax,2), %eax
// NEWSEED-NEXT:  	decl	%eax
// NEWSEED-NEXT:  	cmpl	%edx, %eax
// NEWSEED-NEXT:  	jb	.LBB0_2

// CHECKV2-LABEL: memcpy_xor:
// CHECKV2:     .LBB0_2:
// CHECKV2-NEXT:   	movl	%eax, %ecx
// CHECKV2-NEXT:   	movzbl	(%rsi,%rcx), %r8d
// CHECKV2-NEXT:   	notb	%r8b
// CHECKV2-NEXT:   	movl	%r8d, %r9d
// CHECKV2-NEXT:   	orb	$-36, %r9b
// CHECKV2-NEXT:   	orb	$35, %r8b
// CHECKV2-NEXT:   	subb	%r8b, %r9b
// CHECKV2-NEXT:   	addb	$35, %r9b
// CHECKV2-NEXT:   	movb	%r9b, (%rdi,%rcx)
// CHECKV2-NEXT:   	movl	%eax, %ecx
// CHECKV2-NEXT:   	notl	%ecx
// CHECKV2-NEXT:   	orl	$-2, %ecx
// CHECKV2-NEXT:   	addl	%eax, %ecx
// CHECKV2-NEXT:   	andl	$1, %eax
// CHECKV2-NEXT:   	addl	%ecx, %eax
// CHECKV2-NEXT:   	addl	$2, %eax
// CHECKV2-NEXT:   	cmpl	%edx, %eax
// CHECKV2-NEXT:   	jb	.LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

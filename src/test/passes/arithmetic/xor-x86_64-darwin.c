//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: x86-registered-target && apple_abi

// RUN:                                        clang -target x86_64-apple-darwin                         -O0 -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_0.py clang -target x86_64-apple-darwin -fpass-plugin=%libOMVLL -O0 -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target x86_64-apple-darwin -fpass-plugin=%libOMVLL -fno-verbose-asm -O0 -S %s -o - | FileCheck --check-prefix=R2 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_3.py clang -target x86_64-apple-darwin -fpass-plugin=%libOMVLL -fno-verbose-asm -O0 -S %s -o - | FileCheck --check-prefix=R3 %s

// R0-LABEL: _memcpy_xor:
// R0:           movsbl  (%rax,%rcx), %eax
// R0:           xorl    $35, %eax
// R0:           movb    %al, %dl
// R0:           movq    -8(%rbp), %rax
// R0:           movl    -24(%rbp), %ecx

// R2-LABEL: _memcpy_xor:
// R2:           movsbl	(%rax,%rcx), %eax
// R2-NEXT:     	movl	%eax, %ecx
// R2-NEXT:     	addl	$35, %ecx
// R2-NEXT:     	addl	$1, %ecx
// R2-NEXT:     	movl	%eax, %edx
// R2-NEXT:     	xorl	$-1, %edx
// R2-NEXT:     	movl	$4294967295, %esi
// R2-NEXT:     	xorl	$35, %esi
// R2-NEXT:     	orl	%esi, %edx
// R2-NEXT:     	addl	%edx, %ecx
// R2-NEXT:     	movl	%eax, %esi
// R2-NEXT:     	addl	$35, %esi
// R2-NEXT:     	orl	$35, %eax
// R2-NEXT:     	subl	%eax, %esi
// R2-NEXT:     	xorl	%edx, %edx
// R2-NEXT:     	subl	%esi, %edx
// R2-NEXT:     	movl	%ecx, %eax
// R2-NEXT:     	xorl	%edx, %eax
// R2-NEXT:     	xorl	%edx, %edx
// R2-NEXT:     	subl	%esi, %edx
// R2-NEXT:     	andl	%edx, %ecx
// R2-NEXT:     	shll	%ecx
// R2-NEXT:     	addl	%ecx, %eax
// R2-NEXT:     	movb	%al, %dl
// R2-NEXT:     	movq	-8(%rbp), %rax
// R2-NEXT:     	movl	-24(%rbp), %ecx
// R2-NEXT:     	movb	%dl, (%rax,%rcx)
// R2-NEXT:     	movl	-24(%rbp), %edx
// R2-NEXT:     	movl	%edx, %eax
// R2-NEXT:     	xorl	$-1, %eax
// R2-NEXT:     	orl	$1, %eax
// R2-NEXT:     	movl	%edx, %ecx
// R2-NEXT:     	xorl	$-1, %ecx
// R2-NEXT:     	subl	%ecx, %eax
// R2-NEXT:     	movl	%edx, %ecx
// R2-NEXT:     	xorl	$1, %ecx
// R2-NEXT:     	andl	$1, %edx
// R2-NEXT:     	addl	%edx, %ecx
// R2-NEXT:     	xorl	$-1, %ecx
// R2-NEXT:     	subl	%ecx, %eax
// R2-NEXT:     	subl	$1, %eax
// R2-NEXT:     	movl	%eax, -24(%rbp)

// R3-LABEL: _memcpy_xor:
// R3:      movsbl  (%rax,%rcx), %edx
// R3-NEXT:     movl	%edx, -28(%rbp)
// R3-NEXT:     movl	%edx, %ecx
// R3-NEXT:     andl	$35, %ecx
// R3-NEXT:     movl	%edx, %eax
// R3-NEXT:     orl	$35, %eax
// R3-NEXT:     addl	%eax, %ecx
// R3-NEXT:     movl	$4294967295, %eax
// R3-NEXT:     xorl	$1, %eax
// R3-NEXT:     subl	%eax, %ecx
// R3-NEXT:     subl	$1, %ecx
// R3-NEXT:     movl	%edx, %eax
// R3-NEXT:     orl	$-1, %eax
// R3-NEXT:     movl	%edx, %esi
// R3-NEXT:     andl	$-1, %esi
// R3-NEXT:     subl	%esi, %eax
// R3-NEXT:     movl	$4294967295, %esi
// R3-NEXT:     xorl	$35, %esi
// R3-NEXT:     andl	$-1, %esi
// R3-NEXT:     movl	$35, %edi
// R3-NEXT:     subl	$-1, %edi
// R3-NEXT:     shll	%esi
// R3-NEXT:     addl	%esi, %edi
// R3-NEXT:     movl	%eax, %esi
// R3-NEXT:     xorl	%edi, %esi
// R3-NEXT:     andl	%edi, %eax
// R3-NEXT:     addl	%eax, %esi
// R3-NEXT:     movl	%ecx, %eax
// R3-NEXT:     andl	%esi, %eax
// R3-NEXT:     orl	%esi, %ecx
// R3-NEXT:     addl	%ecx, %eax
// R3-NEXT:     movl	%edx, %esi
// R3-NEXT:     andl	$35, %esi
// R3-NEXT:     movl	%edx, %ecx
// R3-NEXT:     orl	$35, %ecx
// R3-NEXT:     addl	%ecx, %esi
// R3-NEXT:     movl	%edx, %ecx
// R3-NEXT:     xorl	$35, %ecx
// R3-NEXT:     andl	$35, %edx
// R3-NEXT:     addl	%edx, %ecx
// R3-NEXT:     xorl	$-1, %ecx
// R3-NEXT:     addl	%ecx, %esi
// R3-NEXT:     addl	$1, %esi
// R3-NEXT:     movl	%esi, %edi
// R3-NEXT:     xorl	$-1, %edi
// R3-NEXT:     addl	$0, %edi
// R3-NEXT:     addl	$1, %edi
// R3-NEXT:     movl	%eax, %ecx
// R3-NEXT:     orl	%edi, %ecx
// R3-NEXT:     movl	%eax, %edx
// R3-NEXT:     andl	%edi, %edx
// R3-NEXT:     subl	%edx, %ecx
// R3-NEXT:     xorl	$-1, %esi
// R3-NEXT:     addl	$0, %esi
// R3-NEXT:     addl	$1, %esi
// R3-NEXT:     movl	%eax, %edx
// R3-NEXT:     xorl	$-1, %edx
// R3-NEXT:     orl	%esi, %edx
// R3-NEXT:     xorl	$-1, %eax
// R3-NEXT:     subl	%eax, %edx
// R3-NEXT:     shll	%edx
// R3-NEXT:     movl	%ecx, %eax
// R3-NEXT:     andl	%edx, %eax
// R3-NEXT:     orl	%edx, %ecx
// R3-NEXT:     addl	%ecx, %eax
// R3-NEXT:     movb	%al, %dl
// R3-NEXT:     movq	-8(%rbp), %rax
// R3-NEXT:     movl	-24(%rbp), %ecx
// R3-NEXT:     movb	%dl, (%rax,%rcx)
// R3-NEXT:     movl	-24(%rbp), %ecx
// R3-NEXT:     movl	$4294967295, %edx
// R3-NEXT:     xorl	$1, %edx
// R3-NEXT:     movl	$4294967295, %eax
// R3-NEXT:     andl	$1, %eax
// R3-NEXT:     addl	%eax, %edx
// R3-NEXT:     movl	$4294967295, %eax
// R3-NEXT:     xorl	$1, %eax
// R3-NEXT:     orl	$-1, %eax
// R3-NEXT:     movl	$4294967295, %esi
// R3-NEXT:     xorl	$1, %esi
// R3-NEXT:     subl	%esi, %eax
// R3-NEXT:     xorl	$-1, %eax
// R3-NEXT:     addl	%eax, %edx
// R3-NEXT:     addl	$1, %edx
// R3-NEXT:     movl	%edx, %eax
// R3-NEXT:     orl	$-1, %eax
// R3-NEXT:     andl	$-1, %edx
// R3-NEXT:     subl	%edx, %eax
// R3-NEXT:     xorl	$-1, %eax
// R3-NEXT:     subl	%eax, %ecx
// R3-NEXT:     subl	$1, %ecx
// R3-NEXT:     movl	$4294967295, %eax
// R3-NEXT:     xorl	$1, %eax
// R3-NEXT:     subl	%eax, %ecx
// R3-NEXT:     subl	$1, %ecx
// R3-NEXT:     movl	$4294967295, %edx
// R3-NEXT:     orl	$1, %edx
// R3-NEXT:     movl	$4294967295, %eax
// R3-NEXT:     andl	$1, %eax
// R3-NEXT:     subl	%eax, %edx
// R3-NEXT:     movl	%ecx, %eax
// R3-NEXT:     andl	%edx, %eax
// R3-NEXT:     orl	%edx, %ecx
// R3-NEXT:     addl	%ecx, %eax
// R3-NEXT:     movl	$4294967295, %ecx
// R3-NEXT:     xorl	$1, %ecx
// R3-NEXT:     subl	%ecx, %eax
// R3-NEXT:     subl	$1, %eax
// R3-NEXT:     movl	%eax, -24(%rbp)

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

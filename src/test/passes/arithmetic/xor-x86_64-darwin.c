//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: x86-registered-target

// RUN:                                        clang -target x86_64-apple-darwin                         -O1 -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_0.py clang -target x86_64-apple-darwin -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_1.py clang -target x86_64-apple-darwin -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R1 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target x86_64-apple-darwin -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R2 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_3.py clang -target x86_64-apple-darwin -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R3 %s

// R0-LABEL: _memcpy_xor:
// R0:       LBB0_2:
// R0:           movzbl	(%rsi,%rcx), %edx
// R0:           xorb	$35, %dl
// R0:           movb	%dl, (%rdi,%rcx)
// R0:           incq	%rcx
// R0:           cmpq	%rcx, %rax
// R0:           jne	LBB0_2

// FIXME: It seems like rounds=1 doesn't actually do anything!
// R1-LABEL: _memcpy_xor:
// R1:       LBB0_2:
// R1:           movzbl	(%rsi,%rcx), %edx
// R1:           xorb	$35, %dl
// R1:           movb	%dl, (%rdi,%rcx)
// R1:           incq	%rcx
// R1:           cmpq	%rcx, %rax
// R1:           jne	LBB0_2

// R2-LABEL: _memcpy_xor:
// R2:       LBB0_2:
// R2:           movl	%eax, %ecx
// R2:           movsbl	(%rsi,%rcx), %r8d
// R2:           movl	%r8d, %r9d
// R2:           notl	%r9d
// R2:           orl	$-36, %r9d
// R2:           addl	%r8d, %r9d
// R2:           addl	$36, %r9d
// R2:           andl	$35, %r8d
// R2:           negl	%r8d
// R2:           movl	%r9d, %r10d
// R2:           xorl	%r8d, %r10d
// R2:           andl	%r9d, %r8d
// R2:           leal	(%r10,%r8,2), %r8d
// R2:           movb	%r8b, (%rdi,%rcx)
// R2:           movl	%eax, %ecx
// R2:           notl	%ecx
// R2:           orl	$-2, %ecx
// R2:           addl	%eax, %ecx
// R2:           andl	$1, %eax
// R2:           addl	%ecx, %eax
// R2:           addl	$2, %eax
// R2:           cmpl	%edx, %eax
// R2:           jb	LBB0_2

// R3-LABEL: _memcpy_xor:
// R3:       LBB0_2:
// R3:           movl	%eax, %ecx
// R3:           movzbl	(%rsi,%rcx), %r8d
// R3:           leal	35(%r8), %r9d
// R3:           movl	%r8d, %r10d
// R3:           notl	%r10d
// R3:           orl	$-36, %r10d
// R3:           addl	%r8d, %r10d
// R3:           orl	$35, %r8d
// R3:           movl	$-36, %r11d
// R3:           subl	%r10d, %r11d
// R3:           movl	%r11d, %r10d
// R3:           xorl	%r9d, %r10d
// R3:           andl	%r9d, %r11d
// R3:           leal	(%r10,%r11,2), %r9d
// R3:           negl	%r9d
// R3:           movl	%r9d, %r10d
// R3:           xorl	%r8d, %r10d
// R3:           andl	%r8d, %r9d
// R3:           leal	(%r10,%r9,2), %r8d
// R3:           movb	%r8b, (%rdi,%rcx)
// R3:           leal	1(%rax), %ecx
// R3:           movl	%eax, %r8d
// R3:           notl	%r8d
// R3:           orl	$-2, %r8d
// R3:           addl	%eax, %r8d
// R3:           movl	$-2, %r9d
// R3:           subl	%r8d, %r9d
// R3:           movl	%r9d, %r8d
// R3:           xorl	%ecx, %r8d
// R3:           andl	%ecx, %r9d
// R3:           orl	$1, %eax
// R3:           addl	%r8d, %eax
// R3:           leal	(%rax,%r9,2), %eax
// R3:           cmpl	%edx, %eax
// R3:           jb	LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

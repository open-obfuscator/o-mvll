//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: x86-registered-target && android_abi

// RUN:                                        clang -target x86_64-pc-linux-gnu                         -O1 -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_0.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_1.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R1 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target x86_64-pc-linux-gnu -fno-verbose-asm -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R2 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_3.py clang -target x86_64-pc-linux-gnu -fno-verbose-asm -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R3 %s

// R0-LABEL: memcpy_xor:
// R0:       .LBB0_2:
// R0:           movzbl	(%rsi,%rcx), %edx
// R0:           xorb	$35, %dl
// R0:           movb	%dl, (%rdi,%rcx)
// R0:           incq	%rcx
// R0:           cmpq	%rcx, %rax
// R0:           jne	.LBB0_2

// R1-LABEL: memcpy_xor:
// R1:       .LBB0_2:
// R1-NEXT:      movzbl	(%rsi,%rcx), %edx
// R1-NEXT:      xorb	$35, %dl
// R1-NEXT:      movb	%dl, (%rdi,%rcx)
// R1-NEXT:      incq	%rcx
// R1-NEXT:      cmpq	%rcx, %rax
// R1-NEXT:      jne	.LBB0_2

// R2-LABEL: memcpy_xor:
// R2:       .LBB0_2:
// R2-NEXT:    	 movsbl	(%rsi,%rcx), %edx
// R2-NEXT:    	 movl	%edx, %r8d
// R2-NEXT:    	 notl	%r8d
// R2-NEXT:    	 orl	$-36, %r8d
// R2-NEXT:    	 addl	%edx, %r8d
// R2-NEXT:    	 addl	$36, %r8d
// R2-NEXT:    	 andl	$35, %edx
// R2-NEXT:    	 negl	%edx
// R2-NEXT:    	 movl	%r8d, %r9d
// R2-NEXT:    	 xorl	%edx, %r9d
// R2-NEXT:    	 andl	%r8d, %edx
// R2-NEXT:    	 leal	(%r9,%rdx,2), %edx
// R2-NEXT:    	 movb	%dl, (%rdi,%rcx)
// R2-NEXT:    	 incq	%rcx
// R2-NEXT:    	 cmpq	%rcx, %rax
// R2-NEXT:    	 jne	.LBB0_2

// R3-LABEL: memcpy_xor:
// R3:       .LBB0_2:
// R3-NEXT:      movl	%eax, %ecx
// R3-NEXT:      movsbl	(%rsi,%rcx), %r8d
// R3-NEXT:      movl	%r8d, %r9d
// R3-NEXT:      notl	%r9d
// R3-NEXT:      leal	(%r8,%r9,2), %r9d
// R3-NEXT:      incl	%r9d
// R3-NEXT:      orl	$-36, %r9d
// R3-NEXT:      addl	%r8d, %r9d
// R3-NEXT:      addl	$36, %r9d
// R3-NEXT:      leal	35(%r8), %r10d
// R3-NEXT:      orl	$35, %r8d
// R3-NEXT:      negl	%r8d
// R3-NEXT:      movl	%r10d, %r11d
// R3-NEXT:      xorl	%r8d, %r11d
// R3-NEXT:      andl	%r10d, %r8d
// R3-NEXT:      leal	(%r11,%r8,2), %r8d
// R3-NEXT:      negl	%r8d
// R3-NEXT:      movl	%r9d, %r10d
// R3-NEXT:      xorl	%r8d, %r10d
// R3-NEXT:      andl	%r9d, %r8d
// R3-NEXT:      leal	(%r10,%r8,2), %r8d
// R3-NEXT:      movb	%r8b, (%rdi,%rcx)
// R3-NEXT:      leal	(%rax,%rax), %ecx
// R3-NEXT:      andl	$4, %ecx
// R3-NEXT:      addl	%ecx, %eax
// R3-NEXT:      xorl	$4, %ecx
// R3-NEXT:      addl	%ecx, %eax
// R3-NEXT:      addl	$-3, %eax
// R3-NEXT:      cmpl	%edx, %eax
// R3-NEXT:      jb	.LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

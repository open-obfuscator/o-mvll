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
// R1-NEXT:      movl	%edx, %r8d
// R1-NEXT:      notb	%r8b
// R1-NEXT:      addb	%r8b, %r8b
// R1-NEXT:      andb	$70, %r8b
// R1-NEXT:      addb	%dl, %r8b
// R1-NEXT:      addb	$-35, %r8b
// R1-NEXT:      movb	%r8b, (%rdi,%rcx)
// R1-NEXT:      incq	%rcx
// R1-NEXT:      cmpq	%rcx, %rax
// R1-NEXT:      jne	.LBB0_2

// R2-LABEL: memcpy_xor:
// R2:       .LBB0_2:
// R2-NEXT:      movl	%eax, %ecx
// R2-NEXT:      movzbl	(%rsi,%rcx), %r8d
// R2-NEXT:      movl	%r8d, %r9d
// R2-NEXT:      notb	%r9b
// R2-NEXT:      addb	%r9b, %r9b
// R2-NEXT:      andb	$70, %r9b
// R2-NEXT:      addb	%r8b, %r9b
// R2-NEXT:      addb	$-35, %r9b
// R2-NEXT:      movb	%r9b, (%rdi,%rcx)
// R2-NEXT:      movl	%eax, %ecx
// R2-NEXT:      notl	%ecx
// R2-NEXT:      orl	$-2, %ecx
// R2-NEXT:      addl	%eax, %ecx
// R2-NEXT:      andl	$1, %eax
// R2-NEXT:      addl	%ecx, %eax
// R2-NEXT:      addl	$2, %eax
// R2-NEXT:      cmpl	%edx, %eax
// R2-NEXT:      jb	.LBB0_2

// R3-LABEL: memcpy_xor:
// R3:       .LBB0_2:
// R3-NEXT:      movzbl	(%rsi,%rcx), %edx
// R3-NEXT:      movl	%edx, %r8d
// R3-NEXT:      andb	$1, %r8b
// R3-NEXT:      movl	%edx, %r9d
// R3-NEXT:      orb	$1, %r9b
// R3-NEXT:      addb	%r8b, %r9b
// R3-NEXT:      notb	%dl
// R3-NEXT:      addb	%dl, %dl
// R3-NEXT:      andb	$70, %dl
// R3-NEXT:      addb	%r9b, %dl
// R3-NEXT:      addb	$-36, %dl
// R3-NEXT:      movb	%dl, (%rdi,%rcx)
// R3-NEXT:      incq	%rcx
// R3-NEXT:      cmpq	%rcx, %rax
// R3-NEXT:      jne	.LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

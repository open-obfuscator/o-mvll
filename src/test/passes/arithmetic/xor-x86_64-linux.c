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
// R2-NEXT:      movl    %eax, %ecx
// R2-NEXT:      movsbl  (%rsi,%rcx), %r8d
// R2-NEXT:      movl    %r8d, %r9d
// R2-NEXT:      notl    %r9d
// R2-NEXT:      leal    (%r8,%r9,2), %r10d
// R2-NEXT:      leal    (%r8,%r9,2), %r9d
// R2-NEXT:      incl    %r9d
// R2-NEXT:      notl    %r9d
// R2-NEXT:      orl     $35, %r9d
// R2-NEXT:      addl    %r10d, %r9d
// R2-NEXT:      incl    %r9d
// R2-NEXT:      movl    %r8d, %r10d
// R2-NEXT:      xorl    $221, %r10d
// R2-NEXT:      andl    $93, %r8d
// R2-NEXT:      leal    (%r10,%r8,2), %r8d
// R2-NEXT:      leal    (%r8,%r9,2), %r8d
// R2-NEXT:      addb    $2, %r8b
// R2-NEXT:      movb    %r8b, (%rdi,%rcx)
// R2-NEXT:      movl    %eax, %ecx
// R2-NEXT:      xorl    $2, %ecx
// R2-NEXT:      andl    $2, %eax
// R2-NEXT:      leal    (%rcx,%rax,2), %eax
// R2-NEXT:      movl    %eax, %ecx
// R2-NEXT:      notl    %ecx
// R2-NEXT:      leal    (%rcx,%rax,2), %eax
// R2-NEXT:      cmpl    %edx, %eax
// R2-NEXT:      jb      .LBB0_2

// R3-LABEL: memcpy_xor:
// R3:       .LBB0_2:
// R3-NEXT:      movl    %eax, %ecx
// R3-NEXT:      movsbl  (%rsi,%rcx), %r9d
// R3-NEXT:      movl    %r9d, %r8d
// R3-NEXT:      notl    %r8d
// R3-NEXT:      leal    (%r9,%r8,2), %r11d
// R3-NEXT:      leal    (%r9,%r9), %ebx
// R3-NEXT:      movl    %ebx, %r10d
// R3-NEXT:      notl    %r10d
// R3-NEXT:      andl    $186, %r10d
// R3-NEXT:      orl     $2147483613, %r8d
// R3-NEXT:      addl    %r9d, %r8d
// R3-NEXT:      addl    %r9d, %r10d
// R3-NEXT:      xorl    $1, %r9d
// R3-NEXT:      andl    $2, %ebx
// R3-NEXT:      addl    %r9d, %ebx
// R3-NEXT:      leal    (%rbx,%r11,2), %r9d
// R3-NEXT:      leal    (%rbx,%r11,2), %r11d
// R3-NEXT:      addl    $2, %r11d
// R3-NEXT:      notl    %r11d
// R3-NEXT:      leal    (%r9,%r11,2), %ebx
// R3-NEXT:      addl    $2, %ebx
// R3-NEXT:      leal    (%r9,%r11,2), %r9d
// R3-NEXT:      addl    $3, %r9d
// R3-NEXT:      notl    %r9d
// R3-NEXT:      orl     $-36, %r9d
// R3-NEXT:      addl    %ebx, %r9d
// R3-NEXT:      addl    $37, %r9d
// R3-NEXT:      notl    %ebx
// R3-NEXT:      movl    %r9d, %r11d
// R3-NEXT:      xorl    %ebx, %r11d
// R3-NEXT:      andl    %r9d, %ebx
// R3-NEXT:      addl    %r11d, %r8d
// R3-NEXT:      leal    (%r8,%rbx,2), %r8d
// R3-NEXT:      leal    (%r10,%r8,2), %r8d
// R3-NEXT:      addb    $37, %r8b
// R3-NEXT:      movb    %r8b, (%rdi,%rcx)
// R3-NEXT:      leal    (%rax,%rax), %ecx
// R3-NEXT:      andl    $4, %ecx
// R3-NEXT:      leal    (%rax,%rcx), %r8d
// R3-NEXT:      xorl    $4, %ecx
// R3-NEXT:      addl    %r8d, %ecx
// R3-NEXT:      addl    $-2, %ecx
// R3-NEXT:      orl     $1, %ecx
// R3-NEXT:      orl     $-2, %eax
// R3-NEXT:      addl    %ecx, %eax
// R3-NEXT:      cmpl    %edx, %eax
// R3-NEXT:      jb      .LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: x86-registered-target && apple_abi

// RUN:                                        clang -target x86_64-apple-darwin                         -O1 -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_0.py clang -target x86_64-apple-darwin -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target x86_64-apple-darwin -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R2 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_3.py clang -target x86_64-apple-darwin -fpass-plugin=%libOMVLL -fno-verbose-asm -O1 -S %s -o - | FileCheck --check-prefix=R3 %s

// R0-LABEL: _memcpy_xor:
// R0:       LBB0_2:
// R0:           movzbl	(%rsi,%rcx), %edx
// R0:           xorb	$35, %dl
// R0:           movb	%dl, (%rdi,%rcx)
// R0:           incq	%rcx
// R0:           cmpq	%rcx, %rax
// R0:           jne	LBB0_2

// R2-LABEL: _memcpy_xor:
// R2:       LBB0_2:
// R2-NEXT:      movsbl  (%rsi,%rcx), %edx
// R2-NEXT:      movl    %edx, %r8d
// R2-NEXT:      notl    %r8d
// R2-NEXT:      leal    (%rdx,%r8,2), %r9d
// R2-NEXT:      leal    (%rdx,%r8,2), %r8d
// R2-NEXT:      incl    %r8d
// R2-NEXT:      notl    %r8d
// R2-NEXT:      orl     $35, %r8d
// R2-NEXT:      addl    %r9d, %r8d
// R2-NEXT:      incl    %r8d
// R2-NEXT:      movl    %edx, %r9d
// R2-NEXT:      xorl    $221, %r9d
// R2-NEXT:      andl    $93, %edx
// R2-NEXT:      leal    (%r9,%rdx,2), %edx
// R2-NEXT:      leal    (%rdx,%r8,2), %edx
// R2-NEXT:      addb    $2, %dl
// R2-NEXT:      movb    %dl, (%rdi,%rcx)
// R2-NEXT:      incq    %rcx
// R2-NEXT:      cmpq    %rcx, %rax
// R2-NEXT:      jne     LBB0_2

// R3-LABEL: _memcpy_xor:
// R3:       LBB0_2:
// R3-NEXT:      movl    %eax, %ecx
// R3-NEXT:      movsbl  (%rsi,%rcx), %r8d
// R3-NEXT:      movl    %r8d, %r9d
// R3-NEXT:      notl    %r9d
// R3-NEXT:      leal    (%r8,%r9,2), %r10d
// R3-NEXT:      leal    1(%r8,%r9,2), %r9d
// R3-NEXT:      movl    %r9d, %r11d
// R3-NEXT:      notl    %r11d
// R3-NEXT:      leal    (%r10,%r11,2), %r10d
// R3-NEXT:      addl    $2, %r10d
// R3-NEXT:      addl    %r10d, %r9d
// R3-NEXT:      notl    %r10d
// R3-NEXT:      orl     $2147483612, %r10d
// R3-NEXT:      addl    %r9d, %r10d
// R3-NEXT:      leal    (%r8,%r8), %r9d
// R3-NEXT:      andl    $-70, %r9d
// R3-NEXT:      movl    %r9d, %r11d
// R3-NEXT:      xorl    $-70, %r11d
// R3-NEXT:      addl    %r8d, %r11d
// R3-NEXT:      leal    (%r9,%r11), %r8d
// R3-NEXT:      addl    $35, %r8d
// R3-NEXT:      addl    %r9d, %r11d
// R3-NEXT:      leal    74(,%r10,2), %r9d
// R3-NEXT:      movl    %r8d, %r10d
// R3-NEXT:      notl    %r10d
// R3-NEXT:      orl     %r9d, %r10d
// R3-NEXT:      addl    %r11d, %r10d
// R3-NEXT:      orl     %r8d, %r9d
// R3-NEXT:      addl    %r10d, %r9d
// R3-NEXT:      addb    $36, %r9b
// R3-NEXT:      movb    %r9b, (%rdi,%rcx)
// R3-NEXT:      movl    %eax, %ecx
// R3-NEXT:      notl    %ecx
// R3-NEXT:      movl    %ecx, %r8d
// R3-NEXT:      andl    $2, %r8d
// R3-NEXT:      leal    (%rax,%r8,2), %r8d
// R3-NEXT:      orl     $2, %ecx
// R3-NEXT:      addl    %eax, %ecx
// R3-NEXT:      leal    (%r8,%rcx,2), %ecx
// R3-NEXT:      andl    $-2, %ecx
// R3-NEXT:      orl     $-2, %eax
// R3-NEXT:      addl    %ecx, %eax
// R3-NEXT:      incl    %eax
// R3-NEXT:      cmpl    %edx, %eax
// R3-NEXT:      jb      LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

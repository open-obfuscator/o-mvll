// REQUIRES: x86-registered-target

// RUN:                                        clang -target x86_64-pc-linux-gnu                         -O1 -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_0.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_1.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R1 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R2 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_3.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=R3 %s

// R0-LABEL: memcpy_xor:
// R0:       .LBB0_2:
// R0:           movzbl	(%rsi,%rcx), %edx
// R0:           xorb	$35, %dl
// R0:           movb	%dl, (%rdi,%rcx)
// R0:           addq	$1, %rcx
// R0:           cmpq	%rcx, %rax
// R0:           jne	.LBB0_2

// FIXME: It seems like rounds=1 doesn't actually do anything!
// R1-LABEL: memcpy_xor:
// R1:       .LBB0_2:
// R1:           movzbl	(%rsi,%rcx), %edx
// R1:           xorb	$35, %dl
// R1:           movb	%dl, (%rdi,%rcx)
// R1:           addq	$1, %rcx
// R1:           cmpq	%rcx, %rax
// R1:           jne	.LBB0_2

// R2-LABEL: memcpy_xor:
// R2:       .LBB0_2:
// R2:           movl	%r9d, %r8d
// R2:           movsbl	(%rsi,%r8), %eax
// R2:           movl	%eax, %ecx
// R2:           notl	%ecx
// R2:           orl	$-36, %ecx
// R2:           leal	(%rax,%rcx), %r10d
// R2:           addl	$36, %r10d
// R2:           andl	$35, %eax
// R2:           negl	%eax
// R2:           movl	%r10d, %ecx
// R2:           xorl	%eax, %ecx
// R2:           andl	%r10d, %eax
// R2:           leal	(%rcx,%rax,2), %eax
// R2:           movb	%al, (%rdi,%r8)
// R2:           movl	%r9d, %eax
// R2:           notl	%eax
// R2:           orl	$-2, %eax
// R2:           addl	%r9d, %eax
// R2:           movl	%r9d, %ecx
// R2:           andl	$1, %ecx
// R2:           leal	(%rcx,%rax), %r9d
// R2:           addl	$2, %r9d
// R2:           cmpl	%edx, %r9d
// R2:           jb	.LBB0_2

// R3-LABEL: memcpy_xor:
// R3:       .LBB0_2:
// R3:           movl	%eax, %r8d
// R3:           movzbl	(%rsi,%r8), %r10d
// R3:           leal	35(%r10), %r9d
// R3:           movl	%r10d, %ecx
// R3:           notl	%ecx
// R3:           orl	$-36, %ecx
// R3:           addl	%r10d, %ecx
// R3:           orl	$35, %r10d
// R3:           movl	$-36, %r11d
// R3:           subl	%ecx, %r11d
// R3:           movl	%r11d, %ecx
// R3:           xorl	%r9d, %ecx
// R3:           andl	%r9d, %r11d
// R3:           leal	(%rcx,%r11,2), %ecx
// R3:           negl	%ecx
// R3:           movl	%ecx, %r9d
// R3:           xorl	%r10d, %r9d
// R3:           andl	%r10d, %ecx
// R3:           leal	(%r9,%rcx,2), %ecx
// R3:           movb	%cl, (%rdi,%r8)
// R3:           leal	1(%rax), %r8d
// R3:           movl	%eax, %ecx
// R3:           notl	%ecx
// R3:           orl	$-2, %ecx
// R3:           addl	%eax, %ecx
// R3:           movl	$-2, %r9d
// R3:           subl	%ecx, %r9d
// R3:           movl	%r9d, %ecx
// R3:           xorl	%r8d, %ecx
// R3:           andl	%r8d, %r9d
// R3:           orl	$1, %eax
// R3:           addl	%ecx, %eax
// R3:           leal	(%rax,%r9,2), %eax
// R3:           cmpl	%edx, %eax
// R3:           jb	.LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: x86-registered-target && apple_abi

// RUN:                                                                  clang -target x86_64-pc-linux-gnu                         -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_empty_params.py         clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=DEFAULT %s
// RUN: env OMVLL_CONFIG=%S/config_defaultConfig_include_function.py     clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o - | FileCheck --check-prefix=CHECK %s
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
// CHECK-NEXT:      movsbl  (%rsi,%rcx), %edx
// CHECK-NEXT:      movl    %edx, %r8d
// CHECK-NEXT:      orl     $35, %r8d
// CHECK-NEXT:      andl    $35, %edx
// CHECK-NEXT:      negl    %edx
// CHECK-NEXT:      movl    %r8d, %r9d
// CHECK-NEXT:      xorl    %edx, %r9d
// CHECK-NEXT:      andl    %r8d, %edx
// CHECK-NEXT:      leal    (%r9,%rdx,2), %edx
// CHECK-NEXT:      movb    %dl, (%rdi,%rcx)
// CHECK-NEXT:      incq    %rcx
// CHECK-NEXT:      cmpq    %rcx, %rax
// CHECK-NEXT:      jne     .LBB0_2

// This is the exact same config as CHECK (include_function) but with
// a different probability seed, thus the instruction changes
// NEWSEED-LABEL: memcpy_xor:
// NEWSEED:     .LBB0_2:
// NEWSEED:     movl    %eax, %ecx
// NEWSEED:     movsbl  (%rsi,%rcx), %r8d
// NEWSEED:     movl    %r8d, %r9d
// NEWSEED:     xorl    $221, %r9d
// NEWSEED:     addl    %r8d, %r8d
// NEWSEED:     movl    %r8d, %r10d
// NEWSEED:     notl    %r10d
// NEWSEED:     andl    $186, %r8d
// NEWSEED:     addl    %r9d, %r8d
// NEWSEED:     andl    $70, %r10d
// NEWSEED:     addl    %r8d, %r10d
// NEWSEED:     movb    %r10b, (%rdi,%rcx)
// NEWSEED:     movl    %eax, %ecx
// NEWSEED:     andl    $-2, %ecx
// NEWSEED:     addl    %eax, %ecx
// NEWSEED:     notl    %eax
// NEWSEED:     orl     $1, %eax
// NEWSEED:     addl    %ecx, %eax
// NEWSEED:     addl    $2, %eax
// NEWSEED:     cmpl    %edx, %eax
// NEWSEED:     jb      .LBB0_2

// CHECKV2-LABEL: memcpy_xor:
// CHECKV2:     .LBB0_2:
// CHECKV2-NEXT:    movsbl  (%rsi,%rcx), %edx
// CHECKV2-NEXT:    movl    %edx, %r8d
// CHECKV2-NEXT:    xorl    $221, %r8d
// CHECKV2-NEXT:    addl    %edx, %edx
// CHECKV2-NEXT:    movl    %edx, %r9d
// CHECKV2-NEXT:    notl    %r9d
// CHECKV2-NEXT:    andl    $186, %edx
// CHECKV2-NEXT:    addl    %r8d, %edx
// CHECKV2-NEXT:    andl    $70, %r9d
// CHECKV2-NEXT:    addl    %edx, %r9d
// CHECKV2-NEXT:    movb    %r9b, (%rdi,%rcx)
// CHECKV2-NEXT:    incq    %rcx
// CHECKV2-NEXT:    cmpq    %rcx, %rax
// CHECKV2-NEXT:    jne     .LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

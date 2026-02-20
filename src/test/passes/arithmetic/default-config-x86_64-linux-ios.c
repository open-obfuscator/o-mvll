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
// CHECK-NEXT:      notl    %r8d
// CHECK-NEXT:      leal    (%rdx,%r8,2), %r9d
// CHECK-NEXT:      leal    (%rdx,%r8,2), %r8d
// CHECK-NEXT:      incl    %r8d
// CHECK-NEXT:      notl    %r8d
// CHECK-NEXT:      orl     $35, %r8d
// CHECK-NEXT:      addl    %r9d, %r8d
// CHECK-NEXT:      incl    %r8d
// CHECK-NEXT:      movl    %edx, %r9d
// CHECK-NEXT:      xorl    $221, %r9d
// CHECK-NEXT:      andl    $93, %edx
// CHECK-NEXT:      leal    (%r9,%rdx,2), %edx
// CHECK-NEXT:      leal    (%rdx,%r8,2), %edx
// CHECK-NEXT:      addb    $2, %dl
// CHECK-NEXT:      movb    %dl, (%rdi,%rcx)
// CHECK-NEXT:      incq    %rcx
// CHECK-NEXT:      cmpq    %rcx, %rax
// CHECK-NEXT:      jne     .LBB0_2

// This is the exact same config as CHECK (include_function) but with
// a different probability seed, thus the instruction changes
// NEWSEED-LABEL: memcpy_xor:
// NEWSEED:     .LBB0_2
// NEWSEED-NEXT:    movl    %eax, %ecx
// NEWSEED-NEXT:    movzbl  (%rsi,%rcx), %r8d
// NEWSEED-NEXT:    xorb    $35, %r8b
// NEWSEED-NEXT:    movb    %r8b, (%rdi,%rcx)
// NEWSEED-NEXT:    movl    %eax, %ecx
// NEWSEED-NEXT:    xorl    $2, %ecx
// NEWSEED-NEXT:    andl    $2, %eax
// NEWSEED-NEXT:    leal    (%rcx,%rax,2), %eax
// NEWSEED-NEXT:    decl    %eax
// NEWSEED-NEXT:    cmpl    %edx, %eax
// NEWSEED-NEXT:    jb      .LBB0_2

// CHECKV2-LABEL: memcpy_xor:
// CHECKV2:     .LBB0_2:
// CHECKV2-NEXT:    movl    %eax, %ecx
// CHECKV2-NEXT:    movsbl  (%rsi,%rcx), %r8d
// CHECKV2-NEXT:    movl    %r8d, %r9d
// CHECKV2-NEXT:    notl    %r9d
// CHECKV2-NEXT:    movl    %r8d, %r10d
// CHECKV2-NEXT:    shll    $2, %r9d
// CHECKV2-NEXT:    leal    (%r9,%r8,2), %r9d
// CHECKV2-NEXT:    addl    $2, %r9d
// CHECKV2-NEXT:    xorl    $221, %r8d
// CHECKV2-NEXT:    andl    $93, %r10d
// CHECKV2-NEXT:    leal    (%r8,%r10,2), %r8d
// CHECKV2-NEXT:    andl    $70, %r9d
// CHECKV2-NEXT:    addl    %r8d, %r9d
// CHECKV2-NEXT:    movb    %r9b, (%rdi,%rcx)
// CHECKV2-NEXT:    movl    %eax, %ecx
// CHECKV2-NEXT:    notl    %ecx
// CHECKV2-NEXT:    orl     $1, %ecx
// CHECKV2-NEXT:    addl    %eax, %ecx
// CHECKV2-NEXT:    orl     $1, %eax
// CHECKV2-NEXT:    addl    %ecx, %eax
// CHECKV2-NEXT:    incl    %eax
// CHECKV2-NEXT:    cmpl    %edx, %eax
// CHECKV2-NEXT:    jb      .LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

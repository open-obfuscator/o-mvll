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
// CHECK-NEXT:      movl    %eax, %ecx
// CHECK-NEXT:      movsbl  (%rsi,%rcx), %r8d
// CHECK-NEXT:      movl    %r8d, %r9d
// CHECK-NEXT:      notl    %r9d
// CHECK-NEXT:      leal    (%r8,%r9,2), %r10d
// CHECK-NEXT:      leal    (%r8,%r9,2), %r9d
// CHECK-NEXT:      incl    %r9d
// CHECK-NEXT:      notl    %r9d
// CHECK-NEXT:      orl     $35, %r9d
// CHECK-NEXT:      addl    %r10d, %r9d
// CHECK-NEXT:      incl    %r9d
// CHECK-NEXT:      movl    %r8d, %r10d
// CHECK-NEXT:      xorl    $221, %r10d
// CHECK-NEXT:      andl    $93, %r8d
// CHECK-NEXT:      leal    (%r10,%r8,2), %r8d
// CHECK-NEXT:      leal    (%r8,%r9,2), %r8d
// CHECK-NEXT:      addb    $2, %r8b
// CHECK-NEXT:      movb    %r8b, (%rdi,%rcx)
// CHECK-NEXT:      movl    %eax, %ecx
// CHECK-NEXT:      xorl    $2, %ecx
// CHECK-NEXT:      andl    $2, %eax
// CHECK-NEXT:      leal    (%rcx,%rax,2), %eax
// CHECK-NEXT:      movl    %eax, %ecx
// CHECK-NEXT:      notl    %ecx
// CHECK-NEXT:      leal    (%rcx,%rax,2), %eax
// CHECK-NEXT:      cmpl    %edx, %eax
// CHECK-NEXT:      jb      .LBB0_2

// This is the exact same config as CHECK (include_function) but with
// a different probability seed, thus the instruction changes
// NEWSEED-LABEL: memcpy_xor:
// NEWSEED:     .LBB0_2:
// NEWSEED-NEXT:  	movl    %eax, %ecx
// NEWSEED-NEXT:  	movsbl  (%rsi,%rcx), %r8d
// NEWSEED-NEXT:  	movl    %r8d, %r9d
// NEWSEED-NEXT:  	notl    %r9d
// NEWSEED-NEXT:  	leal    (%r8,%r9,2), %r10d
// NEWSEED-NEXT:  	leal    (%r8,%r9,2), %r9d
// NEWSEED-NEXT:  	incl    %r9d
// NEWSEED-NEXT:  	notl    %r9d
// NEWSEED-NEXT:  	orl     $35, %r9d
// NEWSEED-NEXT:  	addl    %r10d, %r9d
// NEWSEED-NEXT:  	incl    %r9d
// NEWSEED-NEXT:  	movl    %r8d, %r10d
// NEWSEED-NEXT:  	xorl    $221, %r10d
// NEWSEED-NEXT:  	andl    $93, %r8d
// NEWSEED-NEXT:  	leal    (%r10,%r8,2), %r8d
// NEWSEED-NEXT:  	leal    (%r8,%r9,2), %r8d
// NEWSEED-NEXT:  	addb    $2, %r8b
// NEWSEED-NEXT:  	movb    %r8b, (%rdi,%rcx)
// NEWSEED-NEXT:  	movl    %eax, %ecx
// NEWSEED-NEXT:  	xorl    $2, %ecx
// NEWSEED-NEXT:  	andl    $2, %eax
// NEWSEED-NEXT:  	leal    (%rcx,%rax,2), %eax
// NEWSEED-NEXT:  	movl    %eax, %ecx
// NEWSEED-NEXT:  	notl    %ecx
// NEWSEED-NEXT:  	leal    (%rcx,%rax,2), %eax
// NEWSEED-NEXT:  	cmpl    %edx, %eax
// NEWSEED-NEXT:  	jb      .LBB0_2

// CHECKV2-LABEL: memcpy_xor:
// CHECKV2:     .LBB0_2:
// CHECKV2-NEXT:   	movzbl  (%rsi,%rcx), %edx
// CHECKV2-NEXT:   	movl    %edx, %r8d
// CHECKV2-NEXT:   	notb    %r8b
// CHECKV2-NEXT:   	addb    %r8b, %r8b
// CHECKV2-NEXT:   	addb    %dl, %r8b
// CHECKV2-NEXT:   	addb    %r8b, %r8b
// CHECKV2-NEXT:   	movl    %edx, %r9d
// CHECKV2-NEXT:   	xorb    $-35, %r9b
// CHECKV2-NEXT:   	addb    %dl, %dl
// CHECKV2-NEXT:   	andb    $-70, %dl
// CHECKV2-NEXT:   	addb    %r9b, %dl
// CHECKV2-NEXT:   	addb    $2, %r8b
// CHECKV2-NEXT:   	andb    $70, %r8b
// CHECKV2-NEXT:   	addb    %dl, %r8b
// CHECKV2-NEXT:   	movb    %r8b, (%rdi,%rcx)
// CHECKV2-NEXT:   	incq    %rcx
// CHECKV2-NEXT:   	cmpq    %rcx, %rax
// CHECKV2-NEXT:   	jne     .LBB0_2

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

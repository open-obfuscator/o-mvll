;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; ShuffleFunctions draws from the shared RandomGenerator, so its ordering is
; driven by omvll.config.probability_seed. Verify two properties:
;   1. The same seed produces identical output on repeated runs (determinism).
;   2. Two different seeds produce different orderings (seed is actually used).

; RUN: rm -rf %t && mkdir -p %t

; Seed 100 twice — outputs must be identical.
; RUN: env OMVLL_CONFIG=%S/config_seed_100.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o %t/seed100_a.ll
; RUN: env OMVLL_CONFIG=%S/config_seed_100.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o %t/seed100_b.ll
; RUN: diff %t/seed100_a.ll %t/seed100_b.ll

; Seed 200 — ordering must differ from seed 100.
; RUN: env OMVLL_CONFIG=%S/config_seed_200.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o %t/seed200.ll
; RUN: grep '^define' %t/seed100_a.ll > %t/seed100.order
; RUN: grep '^define' %t/seed200.ll  > %t/seed200.order
; RUN: not diff %t/seed100.order %t/seed200.order

define i32 @fn_a(i32 %x) { ret i32 %x }
define i32 @fn_b(i32 %x) { ret i32 %x }
define i32 @fn_c(i32 %x) { ret i32 %x }
define i32 @fn_d(i32 %x) { ret i32 %x }
define i32 @fn_e(i32 %x) { ret i32 %x }
define i32 @fn_f(i32 %x) { ret i32 %x }
define i32 @fn_g(i32 %x) { ret i32 %x }
define i32 @fn_h(i32 %x) { ret i32 %x }

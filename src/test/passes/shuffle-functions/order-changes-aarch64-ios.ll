;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; Verify that ShuffleFunctions reorders the module's function list:
;   1. The set of functions is preserved (nothing added or dropped).
;   2. Their order differs from the source order.
;
; The baseline is produced with the pass disabled, which leaves the functions
; in source order. Eight independent functions make an accidental identity
; permutation of the fixed-seed shuffle effectively impossible.

; RUN: rm -rf %t && mkdir -p %t

; RUN: env OMVLL_CONFIG=%S/config_disabled.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o %t/baseline.ll
; RUN: env OMVLL_CONFIG=%S/config_enabled.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o %t/shuffled.ll

; Same set of functions, but a different order.
; RUN: grep '^define' %t/baseline.ll > %t/baseline.order
; RUN: grep '^define' %t/shuffled.ll > %t/shuffled.order
; RUN: not diff %t/baseline.order %t/shuffled.order
; RUN: sort %t/baseline.order -o %t/baseline.sorted
; RUN: sort %t/shuffled.order -o %t/shuffled.sorted
; RUN: diff %t/baseline.sorted %t/shuffled.sorted

define i32 @fn_a(i32 %x) { ret i32 %x }
define i32 @fn_b(i32 %x) { ret i32 %x }
define i32 @fn_c(i32 %x) { ret i32 %x }
define i32 @fn_d(i32 %x) { ret i32 %x }
define i32 @fn_e(i32 %x) { ret i32 %x }
define i32 @fn_f(i32 %x) { ret i32 %x }
define i32 @fn_g(i32 %x) { ret i32 %x }
define i32 @fn_h(i32 %x) { ret i32 %x }

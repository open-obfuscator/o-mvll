;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; RUN: env OMVLL_CONFIG=%S/config_all.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK-O0 %s

; RUN: env OMVLL_CONFIG=%S/config_all.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O1 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK-O1 %s

define i32 @opaque_constants(i32 %val) {
; CHECK-LABEL:  define i32 @opaque_constants(
; CHECK-LABEL:  entry
; CHECK-O0:       %opaque.t1 = alloca i64, align 8
; CHECK-O0-NEXT:  %opaque.t2 = alloca i64, align 8
; CHECK-O0-NEXT:  %0 = ptrtoint ptr %opaque.t2 to i64
; CHECK-O0-NEXT:  store i64 %0, ptr %opaque.t1, align 8
; CHECK-O0-NEXT:  %1 = ptrtoint ptr %opaque.t1 to i64
; CHECK-O0-NEXT:  store i64 %1, ptr %opaque.t2, align 8
; CHECK-O0-NEXT:  %2 = load volatile i32, ptr %opaque.t1, align 4
; CHECK-O0-NEXT:  %3 = load volatile i32, ptr %opaque.t2, align 4
; CHECK-O0-NEXT:  %4 = xor i32 %2, %3, !obf !0
; CHECK-O0-NEXT:  %5 = xor i32 %3, %2, !obf !0
; CHECK-O0-NEXT:  %6 = sub i32 %4, %5, !obf !0
; CHECK-O0-NEXT:  %7 = add i32 %6, -1323891279, !obf !0
; CHECK-O0-NEXT:  %8 = add i32 %6, 588839503, !obf !0
; CHECK-O0-NEXT:  store volatile i32 %7, ptr %opaque.t2, align 4
; CHECK-O0-NEXT:  store volatile i32 %8, ptr %opaque.t1, align 4
; CHECK-O0-NEXT:  %9 = load volatile i32, ptr %opaque.t2, align 4
; CHECK-O0-NEXT:  %10 = load volatile i32, ptr %opaque.t1, align 4
; CHECK-O0-NEXT:  %11 = add i32 %9, %10, !obf !0
; CHECK-O0-NEXT:  %cmp = icmp eq i32 %val, %11

; CHECK-O1:       %opaque.t1 = alloca i64, align 8
; CHECK-O1-NEXT:  %opaque.t2 = alloca i64, align 8
; CHECK-O1-NEXT:  %0 = ptrtoint ptr %opaque.t2 to i64
; CHECK-O1-NEXT:  store i64 %0, ptr %opaque.t1, align 8
; CHECK-O1-NEXT:  %1 = ptrtoint ptr %opaque.t1 to i64
; CHECK-O1-NEXT:  store i64 %1, ptr %opaque.t2, align 8
; CHECK-O1-NEXT:  %2 = load volatile i32, ptr %opaque.t1, align 8
; CHECK-O1-NEXT:  %3 = load volatile i32, ptr %opaque.t2, align 8
; CHECK-O1-NEXT:  store volatile i32 -1323891279, ptr %opaque.t2, align 8
; CHECK-O1-NEXT:  store volatile i32 588839503, ptr %opaque.t1, align 8
; CHECK-O1-NEXT:  %4 = load volatile i32, ptr %opaque.t2, align 8
; CHECK-O1-NEXT:  %5 = load volatile i32, ptr %opaque.t1, align 8
; CHECK-O1-NEXT:  %6 = add i32 %5, %4, !obf !0
; CHECK-O1-NEXT:  %cmp = icmp eq i32 %6, %val
entry:
  %cmp = icmp eq i32 %val, -735051776
  br i1 %cmp, label %is_brk, label %not_brk

is_brk:
  ret i32 1

not_brk:
  ret i32 0
}

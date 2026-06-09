;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; RUN: env OMVLL_CONFIG=%S/config_all_routine_1.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK-R1-O0 %s

; RUN: env OMVLL_CONFIG=%S/config_all_routine_1.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O1 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK-R1-O1 %s

; RUN: env OMVLL_CONFIG=%S/config_all_routine_2.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK-R2-O0 %s

; RUN: env OMVLL_CONFIG=%S/config_all_routine_2.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O1 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK-R2-O1 %s

; RUN: env OMVLL_CONFIG=%S/config_all_routine_3.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK-R3-O0 %s

; RUN: env OMVLL_CONFIG=%S/config_all_routine_3.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O1 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK-R3-O1 %s

define i32 @opaque_constants(i32 %val) {
; CHECK-LABEL:  define i32 @opaque_constants(
; CHECK-LABEL:  entry
; CHECK-R1-O0:       %opaque.t1 = alloca i64, align 8
; CHECK-R1-O0-NEXT:  %opaque.t2 = alloca i64, align 8
; CHECK-R1-O0-NEXT:  %0 = ptrtoint ptr %opaque.t2 to i64
; CHECK-R1-O0-NEXT:  store i64 %0, ptr %opaque.t1, align 8
; CHECK-R1-O0-NEXT:  %1 = ptrtoint ptr %opaque.t1 to i64
; CHECK-R1-O0-NEXT:  store i64 %1, ptr %opaque.t2, align 8
; CHECK-R1-O0-NEXT:  %2 = load volatile i32, ptr %opaque.t1, align 4
; CHECK-R1-O0-NEXT:  %3 = load volatile i32, ptr %opaque.t2, align 4
; CHECK-R1-O0-NEXT:  %4 = xor i32 %2, %3, !obf !0
; CHECK-R1-O0-NEXT:  %5 = xor i32 %3, %2, !obf !0
; CHECK-R1-O0-NEXT:  %6 = sub i32 %4, %5, !obf !0
; CHECK-R1-O0-NEXT:  %7 = add i32 %6, 415731863, !obf !0
; CHECK-R1-O0-NEXT:  %8 = add i32 %6, -1150783639, !obf !0
; CHECK-R1-O0-NEXT:  store volatile i32 %7, ptr %opaque.t2, align 4
; CHECK-R1-O0-NEXT:  store volatile i32 %8, ptr %opaque.t1, align 4
; CHECK-R1-O0-NEXT:  %9 = load volatile i32, ptr %opaque.t2, align 4
; CHECK-R1-O0-NEXT:  %10 = load volatile i32, ptr %opaque.t1, align 4
; CHECK-R1-O0-NEXT:  %11 = add i32 %9, %10, !obf !0
; CHECK-R1-O0-NEXT:  %cmp = icmp eq i32 %val, %11

; CHECK-R1-O1:        %opaque.t1 = alloca i64, align 8
; CHECK-R1-O1-NEXT:   %opaque.t2 = alloca i64, align 8
; CHECK-R1-O1-NEXT:   %0 = ptrtoint ptr %opaque.t2 to i64
; CHECK-R1-O1-NEXT:   store i64 %0, ptr %opaque.t1, align 8
; CHECK-R1-O1-NEXT:   %1 = ptrtoint ptr %opaque.t1 to i64
; CHECK-R1-O1-NEXT:   store i64 %1, ptr %opaque.t2, align 8
; CHECK-R1-O1-NEXT:   %2 = load volatile i32, ptr %opaque.t1, align 8
; CHECK-R1-O1-NEXT:   %3 = load volatile i32, ptr %opaque.t2, align 8
; CHECK-R1-O1-NEXT:   store volatile i32 415731863, ptr %opaque.t2, align 8
; CHECK-R1-O1-NEXT:   store volatile i32 -1150783639, ptr %opaque.t1, align 8
; CHECK-R1-O1-NEXT:   %4 = load volatile i32, ptr %opaque.t2, align 8
; CHECK-R1-O1-NEXT:   %5 = load volatile i32, ptr %opaque.t1, align 8
; CHECK-R1-O1-NEXT:   %6 = add i32 %5, %4, !obf !0
; CHECK-R1-O1-NEXT:   %cmp = icmp eq i32 %val, %6

; CHECK-R2-O0:       %opaque.t1 = alloca i64, align 8
; CHECK-R2-O0-NEXT:  %opaque.t2 = alloca i64, align 8
; CHECK-R2-O0-NEXT:  %0 = ptrtoint ptr %opaque.t2 to i64
; CHECK-R2-O0-NEXT:  store i64 %0, ptr %opaque.t1, align 8
; CHECK-R2-O0-NEXT:  %1 = ptrtoint ptr %opaque.t1 to i64
; CHECK-R2-O0-NEXT:  store i64 %1, ptr %opaque.t2, align 8
; CHECK-R2-O0-NEXT:  %2 = load volatile i32, ptr %opaque.t1, align 4
; CHECK-R2-O0-NEXT:  %3 = load volatile i32, ptr %opaque.t2, align 4
; CHECK-R2-O0-NEXT:  %4 = xor i32 %2, %3, !obf !0
; CHECK-R2-O0-NEXT:  %5 = xor i32 -1150783639, %4, !obf !0
; CHECK-R2-O0-NEXT:  %6 = xor i32 %5, %4, !obf !0
; CHECK-R2-O0-NEXT:  %7 = add i32 %4, 411, !obf !0
; CHECK-R2-O0-NEXT:  %8 = icmp eq i32 %7, 143, !obf !0
; CHECK-R2-O0-NEXT:  %9 = select i1 %8, i32 -1150783639, i32 %6, !obf !0
; CHECK-R2-O0-NEXT:  %10 = select i1 %8, i32 51968591, i32 51968591, !obf !0
; CHECK-R2-O0-NEXT:  %11 = select i1 %8, i32 363763272, i32 363763272, !obf !0
; CHECK-R2-O0-NEXT:  store volatile i32 %9, ptr %opaque.t1, align 4
; CHECK-R2-O0-NEXT:  store volatile i32 %10, ptr %opaque.t2, align 4
; CHECK-R2-O0-NEXT:  %12 = load volatile i32, ptr %opaque.t2, align 4
; CHECK-R2-O0-NEXT:  %13 = load volatile i32, ptr %opaque.t1, align 4
; CHECK-R2-O0-NEXT:  %14 = add i32 %12, %13, !obf !0
; CHECK-R2-O0-NEXT:  %15 = add i32 %14, %11, !obf !0
; CHECK-R2-O0-NEXT:  %cmp = icmp eq i32 %val, %15

; CHECK-R2-O1:       %opaque.t1 = alloca i64, align 8
; CHECK-R2-O1-NEXT:  %opaque.t2 = alloca i64, align 8
; CHECK-R2-O1-NEXT:  %0 = ptrtoint ptr %opaque.t2 to i64
; CHECK-R2-O1-NEXT:  store i64 %0, ptr %opaque.t1, align 8
; CHECK-R2-O1-NEXT:  %1 = ptrtoint ptr %opaque.t1 to i64
; CHECK-R2-O1-NEXT:  store i64 %1, ptr %opaque.t2, align 8
; CHECK-R2-O1-NEXT:  %2 = load volatile i32, ptr %opaque.t1, align 8
; CHECK-R2-O1-NEXT:  %3 = load volatile i32, ptr %opaque.t2, align 8
; CHECK-R2-O1-NEXT:  store volatile i32 -1150783639, ptr %opaque.t1, align 8
; CHECK-R2-O1-NEXT:  store volatile i32 51968591, ptr %opaque.t2, align 8
; CHECK-R2-O1-NEXT:  %4 = load volatile i32, ptr %opaque.t2, align 8
; CHECK-R2-O1-NEXT:  %5 = load volatile i32, ptr %opaque.t1, align 8
; CHECK-R2-O1-NEXT:  %6 = add i32 %4, 363763272, !obf !0
; CHECK-R2-O1-NEXT:  %7 = add i32 %6, %5, !obf !0
; CHECK-R2-O1-NEXT:  %cmp = icmp eq i32 %val, %7

; CHECK-R3-O0:       %opaque.t1 = alloca i64, align 8
; CHECK-R3-O0-NEXT:  %opaque.t2 = alloca i64, align 8
; CHECK-R3-O0-NEXT:  %0 = ptrtoint ptr %opaque.t2 to i64
; CHECK-R3-O0-NEXT:  store i64 %0, ptr %opaque.t1, align 8
; CHECK-R3-O0-NEXT:  %1 = ptrtoint ptr %opaque.t1 to i64
; CHECK-R3-O0-NEXT:  store i64 %1, ptr %opaque.t2, align 8
; CHECK-R3-O0-NEXT:  %2 = load volatile i32, ptr %opaque.t1, align 4
; CHECK-R3-O0-NEXT:  %3 = load volatile i32, ptr %opaque.t2, align 4
; CHECK-R3-O0-NEXT:  %4 = xor i32 %2, -1150783639, !obf !0
; CHECK-R3-O0-NEXT:  %5 = add i32 %3, 588839503, !obf !0
; CHECK-R3-O0-NEXT:  %6 = xor i32 -735051776, %4, !obf !0
; CHECK-R3-O0-NEXT:  %7 = add i32 %6, %5, !obf !0
; CHECK-R3-O0-NEXT:  %8 = sub i32 %7, %5, !obf !0
; CHECK-R3-O0-NEXT:  %9 = xor i32 %8, %4, !obf !0
; CHECK-R3-O0-NEXT:  store volatile i32 %9, ptr %opaque.t1, align 4
; CHECK-R3-O0-NEXT:  %10 = load volatile i32, ptr %opaque.t1, align 4
; CHECK-R3-O0-NEXT:  %cmp = icmp eq i32 %val, %10

; CHECK-R3-O1:       %opaque.t1 = alloca i64, align 8
; CHECK-R3-O1-NEXT:  %opaque.t2 = alloca i64, align 8
; CHECK-R3-O1-NEXT:  %0 = ptrtoint ptr %opaque.t2 to i64
; CHECK-R3-O1-NEXT:  store i64 %0, ptr %opaque.t1, align 8
; CHECK-R3-O1-NEXT:  %1 = ptrtoint ptr %opaque.t1 to i64
; CHECK-R3-O1-NEXT:  store i64 %1, ptr %opaque.t2, align 8
; CHECK-R3-O1-NEXT:  %2 = load volatile i32, ptr %opaque.t1, align 8
; CHECK-R3-O1-NEXT:  %3 = load volatile i32, ptr %opaque.t2, align 8
; CHECK-R3-O1-NEXT:  store volatile i32 -735051776, ptr %opaque.t1, align 8
; CHECK-R3-O1-NEXT:  %4 = load volatile i32, ptr %opaque.t1, align 8
; CHECK-R3-O1-NEXT:  %cmp = icmp eq i32 %val, %4
; CHECK-R3-O1-NEXT:  %. = zext i1 %cmp to i32
; CHECK-R3-O1-NEXT:  ret i32 %.

entry:
  %cmp = icmp eq i32 %val, -735051776
  br i1 %cmp, label %is_brk, label %not_brk

is_brk:
  ret i32 1

not_brk:
  ret i32 0
}

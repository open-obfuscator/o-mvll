;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; RUN: env OMVLL_CONFIG=%S/config_all.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK-O0 %s

; RUN: env OMVLL_CONFIG=%S/config_all.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O1 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK-O1 %s

define i32 @opaque_constants() {
; CHECK-LABEL:  define i32 @opaque_constants(
; CHECK-LABEL:  entry
; CHECK-O0:       %opaque.t1 = alloca i64, align 8
; CHECK-O0-NEXT:  %opaque.t2 = alloca i64, align 8
; CHECK-O0-NEXT:  %0 = ptrtoint ptr %opaque.t2 to i64
; CHECK-O0-NEXT:  store i64 %0, ptr %opaque.t1, align 8
; CHECK-O0-NEXT:  %1 = ptrtoint ptr %opaque.t1 to i64
; CHECK-O0-NEXT:  store i64 %1, ptr %opaque.t2, align 8
; CHECK-O0-NEXT:  %stack.0 = alloca i32, align 4
; CHECK-O0-NEXT:  %stack.1 = alloca i32, align 4
; CHECK-O0-NEXT:  %stack.2 = alloca i32, align 4
; CHECK-O0-NEXT:  %2 = load volatile i32, ptr %opaque.t1, align 4
; CHECK-O0-NEXT:  %3 = load volatile i32, ptr %opaque.t2, align 4
; CHECK-O0-NEXT:  %4 = xor i32 %2, %3, !obf !0
; CHECK-O0-NEXT:  %5 = xor i32 %4, %2, !obf !0
; CHECK-O0-NEXT:  %6 = xor i32 %4, %3, !obf !0
; CHECK-O0-NEXT:  %7 = xor i32 %5, %6, !obf !0
; CHECK-O0-NEXT:  %8 = xor i32 %6, %5, !obf !0
; CHECK-O0-NEXT:  %9 = sub i32 %7, %8, !obf !0
; CHECK-O0-NEXT:  store i32 %9, ptr %stack.0, align 4
; CHECK-O0-NEXT:  %10 = load volatile i32, ptr %opaque.t2, align 4
; CHECK-O0-NEXT:  %11 = xor i32 67, %10, !obf !0
; CHECK-O0-NEXT:  %12 = xor i32 %11, %10, !obf !0
; CHECK-O0-NEXT:  %13 = or i32 %12, 1, !obf !0
; CHECK-O0-NEXT:  %14 = and i32 %13, 1, !obf !0
; CHECK-O0-NEXT:  store i32 %14, ptr %stack.1, align 4
; CHECK-O0-NEXT:  %val.0 = load i32, ptr %stack.0, align 4
; CHECK-O0-NEXT:  %val.1 = load i32, ptr %stack.1, align 4
; CHECK-O0-NEXT:  %sum.0 = add i32 %val.1, %val.0
; CHECK-O0-NEXT:  %15 = load volatile i32, ptr %opaque.t1, align 4
; CHECK-O0-NEXT:  %16 = load volatile i32, ptr %opaque.t2, align 4
; CHECK-O0-NEXT:  %17 = xor i32 %15, %16, !obf !0
; CHECK-O0-NEXT:  %18 = xor i32 %16, %15, !obf !0
; CHECK-O0-NEXT:  %19 = sub i32 %17, %18, !obf !0
; CHECK-O0-NEXT:  %20 = add i32 %19, 2, !obf !0
; CHECK-O0-NEXT:  %21 = add i32 %19, 1, !obf !0
; CHECK-O0-NEXT:  store volatile i32 %20, ptr %opaque.t2, align 4
; CHECK-O0-NEXT:  store volatile i32 %21, ptr %opaque.t1, align 4
; CHECK-O0-NEXT:  %22 = load volatile i32, ptr %opaque.t2, align 4
; CHECK-O0-NEXT:  %23 = load volatile i32, ptr %opaque.t1, align 4
; CHECK-O0-NEXT:  %24 = add i32 %22, %23, !obf !0
; CHECK-O0-NEXT:  %sum.1 = add i32 %24, %sum.0
; CHECK-O0-NEXT:  ret i32 %sum.1

; CHECK-O1:       %opaque.t1 = alloca i64, align 8
; CHECK-O1-NEXT:  %opaque.t2 = alloca i64, align 8
; CHECK-O1-NEXT:  %0 = ptrtoint ptr %opaque.t2 to i64
; CHECK-O1-NEXT:  store i64 %0, ptr %opaque.t1, align 8
; CHECK-O1-NEXT:  %1 = ptrtoint ptr %opaque.t1 to i64
; CHECK-O1-NEXT:  store i64 %1, ptr %opaque.t2, align 8
; CHECK-O1-NEXT:  %2 = load volatile i32, ptr %opaque.t1, align 8
; CHECK-O1-NEXT:  %3 = load volatile i32, ptr %opaque.t2, align 8
; CHECK-O1-NEXT:  store volatile i32 2, ptr %opaque.t2, align 8
; CHECK-O1-NEXT:  store volatile i32 2, ptr %opaque.t1, align 8
; CHECK-O1-NEXT:  %4 = load volatile i32, ptr %opaque.t2, align 8
; CHECK-O1-NEXT:  %5 = load volatile i32, ptr %opaque.t1, align 8
; CHECK-O1-NEXT:  %6 = add i32 %5, %4, !obf !0
; CHECK-O1-NEXT:  ret i32 %6
entry:
  %stack.0 = alloca i32, align 4
  %stack.1 = alloca i32, align 4
  %stack.2 = alloca i32, align 4
  store i32 0, ptr %stack.0, align 4
  store i32 1, ptr %stack.1, align 4
  %val.0 = load i32, ptr %stack.0, align 4
  %val.1 = load i32, ptr %stack.1, align 4
  %sum.0 = add i32 %val.1, %val.0
  %sum.1 = add i32 3, %sum.0
  ret i32 %sum.1
}

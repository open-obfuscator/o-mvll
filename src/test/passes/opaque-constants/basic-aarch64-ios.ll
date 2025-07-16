;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; RUN: env OMVLL_CONFIG=%S/config_all.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o - | FileCheck %s

define i32 @opaque_constants() {
; CHECK-LABEL:  define i32 @opaque_constants(
; CHECK-LABEL:  entry
; CHECK:          %0 = alloca i32, align 4
; CHECK-NEXT:     %1 = load i32, ptr %0, align 4
; CHECK-NEXT:     %2 = xor i32 {{.*}}, %1, !obf !0
; CHECK-NEXT:     %3 = xor i32 %2, %1, !obf !0
; CHECK-NEXT:     %4 = xor i32 %2, %1, !obf !0
; CHECK-NEXT:     %5 = xor i32 %3, %4, !obf !0
; CHECK-NEXT:     %6 = xor i32 %4, %3, !obf !0
; CHECK-NEXT:     %7 = sub i32 %5, %6, !obf !0
; CHECK-NEXT:     store i32 %7, ptr %stack.0, align 4
; CHECK-NEXT:     %8 = alloca i32, align 4
; CHECK-NEXT:     %9 = load i32, ptr %8, align 4
; CHECK-NEXT:     %10 = xor i32 7, %9, !obf !0
; CHECK-NEXT:     %11 = xor i32 %10, %9, !obf !0
; CHECK-NEXT:     %12 = or i32 %11, 1, !obf !0
; CHECK-NEXT:     %13 = and i32 %12, 1, !obf !0
; CHECK-NEXT:     store i32 %13, ptr %stack.1, align 4
; CHECK-NEXT:     %val.0 = load i32, ptr %stack.0, align 4
; CHECK-NEXT:     %val.1 = load i32, ptr %stack.1, align 4
; CHECK-NEXT:     %sum.0 = add i32 %val.1, %val.0
; CHECK-NEXT:     %14 = icmp eq i32 1, 1, !obf !0
; CHECK-NEXT:     %15 = xor i32 1, 0, !obf !0
; CHECK-NEXT:     %16 = xor i32 2, 0, !obf !0
; CHECK-NEXT:     %17 = select i1 %14, i32 %15, i32 1, !obf !0
; CHECK-NEXT:     %18 = select i1 %14, i32 %16, i32 2, !obf !0
; CHECK-NEXT:     %19 = add i32 %17, %18, !obf !0
; CHECK-NEXT:     %sum.1 = add i32 %19, %sum.0
; CHECK-NEXT:     ret i32 %sum.1

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

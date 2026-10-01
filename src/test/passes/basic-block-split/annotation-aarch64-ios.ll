;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; RUN: env OMVLL_CONFIG=%S/config_annotation.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK %s

; The baseline probability is 0, so the source-level `bbsplit` annotation is the
; only thing that enables the pass. `included` carries `bbsplit` and must be
; split; `excluded` carries `!bbsplit` and must be left untouched even though it
; is otherwise identical.

; The `bbsplit`-annotated function is split down the middle.
; CHECK-LABEL: define i32 @included(i32 %a, i32 %b)
; CHECK:       work:
; CHECK-NEXT:    %w1 = add i32 %a, %b
; CHECK-NEXT:    %w2 = mul i32 %w1, %a
; CHECK-NEXT:    br label %[[SPLIT:.*]]
; CHECK:       [[SPLIT]]:
; CHECK-NEXT:    %w3 = sub i32 %w2, %b
; CHECK-NEXT:    %w4 = xor i32 %w3, %w1
; CHECK-NEXT:    ret i32 %w4

; The `!bbsplit`-annotated function keeps its `work` block intact (no split).
; CHECK-LABEL: define i32 @excluded(i32 %a, i32 %b)
; CHECK:       work:
; CHECK-NEXT:    %w1 = add i32 %a, %b
; CHECK-NEXT:    %w2 = mul i32 %w1, %a
; CHECK-NEXT:    %w3 = sub i32 %w2, %b
; CHECK-NEXT:    %w4 = xor i32 %w3, %w1
; CHECK-NEXT:    ret i32 %w4

; Annotation string globals.
@.str.in = private unnamed_addr constant [8 x i8] c"bbsplit\00", section "llvm.metadata"
@.str.out = private unnamed_addr constant [9 x i8] c"!bbsplit\00", section "llvm.metadata"
@.str.file = private unnamed_addr constant [6 x i8] c"t.cpp\00", section "llvm.metadata"

@llvm.global.annotations = appending global [2 x { ptr, ptr, ptr, i32, ptr }] [
  { ptr, ptr, ptr, i32, ptr } { ptr @included, ptr @.str.in, ptr @.str.file, i32 1, ptr null },
  { ptr, ptr, ptr, i32, ptr } { ptr @excluded, ptr @.str.out, ptr @.str.file, i32 2, ptr null }
], section "llvm.metadata"

define i32 @included(i32 %a, i32 %b) {
entry:
  br label %work

work:
  %w1 = add i32 %a, %b
  %w2 = mul i32 %w1, %a
  %w3 = sub i32 %w2, %b
  %w4 = xor i32 %w3, %w1
  ret i32 %w4
}

define i32 @excluded(i32 %a, i32 %b) {
entry:
  br label %work

work:
  %w1 = add i32 %a, %b
  %w2 = mul i32 %w1, %a
  %w3 = sub i32 %w2, %b
  %w4 = xor i32 %w3, %w1
  ret i32 %w4
}

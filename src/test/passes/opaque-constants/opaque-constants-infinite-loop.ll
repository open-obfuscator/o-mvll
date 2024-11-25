;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; RUN: env OMVLL_CONFIG=%S/config_all.py clang -target aarch64-linux-android  -fpass-plugin=%libOMVLL -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=O0
; RUN: env OMVLL_CONFIG=%S/config_all.py clang -target aarch64-linux-android  -fpass-plugin=%libOMVLL -O1 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=O1
; RUN: env OMVLL_CONFIG=%S/config_all.py clang -target arm64-apple-ios        -fpass-plugin=%libOMVLL -O0 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=O0
; RUN: env OMVLL_CONFIG=%S/config_all.py clang -target arm64-apple-ios        -fpass-plugin=%libOMVLL -O1 -S -emit-llvm %s -o - | FileCheck %s --check-prefix=O1

define void @opaque_infinite_loop() {
; O0-LABEL: @opaque_infinite_loop
; O0: loop
; O0: %loaded_i = load i32, ptr %i, align 4
; O0-NEXT: %add = add nsw i32 %loaded_i, 1
; O0-NEXT: store i32 %add, ptr %i, align 4
; O0-NEXT: br label %loop
; O1-LABEL: @opaque_infinite_loop
; O1: loop
; O1: br label %loop
entry:
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %loop
loop:                                                ; preds = %loop, %entry
  %loaded_i = load i32, ptr %i, align 4
  %add = add nsw i32 %loaded_i, 1
  store i32 %add, ptr %i, align 4
  br label %loop
}

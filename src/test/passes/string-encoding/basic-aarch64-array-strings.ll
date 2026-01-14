;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target

;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target arm64-apple-ios26.0.0 -O1 -S -emit-llvm %s -o - | FileCheck %s
;
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target aarch64-linux-android -O1 -S -emit-llvm %s -o - | FileCheck %s
;
;     CHECK-NOT:     {{1Hello.*}}
;     CHECK-NOT:     {{2Hello.*}}

@.str.1 = private constant [7 x i8] c"1Hello\00", align 1
@.str.2 = private constant [7 x i8] c"2Hello\00", align 1
@__const.array = private constant [2 x ptr] [ptr @.str.1, ptr @.str.2], align 8

define void @copy(ptr %p) {
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %p, ptr align 8 @__const.array, i64 16, i1 false)
  ret void
}

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)

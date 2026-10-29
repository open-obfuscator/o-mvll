;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target

; @.str.shared is read directly by @direct and also listed in @__const.table.
; @direct is visited first: the table initializer takes the literal's address,
; so it is encoded globally (decoded in place by a module constructor) although
; the config asks for local encoding. @.str.only is reached through the table
; alone, so it stays local and makes @lookup rebuild the table around its decode
; buffer. That rebuild must leave the globally encoded entry naming the literal
; itself: global decoding never writes a buffer, so a buffer there would stay
; zeroed and the entry would read as "".
;
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target arm64-apple-ios26.0.0 -O1 -S -emit-llvm %s -o - | FileCheck %s
;
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target aarch64-linux-android -O1 -S -emit-llvm %s -o - | FileCheck %s
;
;     CHECK-NOT: {{.*HelloShared.*}}
;     CHECK-NOT: {{.*HelloOnly.*}}

; Encoded in place, so it loses `constant`.
@.str.shared = private unnamed_addr constant [12 x i8] c"HelloShared\00", align 1
; CHECK: @.str.shared = private unnamed_addr global [12 x i8] c"

@.str.only = private unnamed_addr constant [10 x i8] c"HelloOnly\00", align 1
@__const.table = private unnamed_addr constant [2 x ptr] [ptr @.str.shared, ptr @.str.only], align 8

; A single decode buffer, for @.str.only: none for @.str.shared.
; CHECK-NOT: internal global [12 x i8] zeroinitializer
; CHECK: @[[BUF:[0-9]+]] = internal global [10 x i8] zeroinitializer
; CHECK-NOT: internal global [12 x i8] zeroinitializer
; CHECK: @[[TABLE:[0-9]+]] = internal unnamed_addr constant [2 x ptr] [ptr @.str.shared, ptr @[[BUF]]]
; CHECK: @llvm.global_ctors = {{.*}}@__omvll_ctor_

define ptr @direct() {
  ret ptr @.str.shared
}

; @.str.only is decoded into the buffer the table now names.
; CHECK-LABEL: define {{.*}}void @lookup(
; CHECK:         getelementptr inbounds nuw i8, ptr @.str.only
; CHECK:         getelementptr inbounds nuw i8, ptr @[[BUF]]
; CHECK:         tail call void @llvm.memcpy.p0.p0.i64(ptr {{.*}}%p, ptr {{.*}}@[[TABLE]], i64 16, i1 false)
define void @lookup(ptr %p) {
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %p, ptr align 8 @__const.table, i64 16, i1 false)
  ret void
}

; @.str.shared is decoded where it lies.
; CHECK-LABEL: define private void @__omvll_ctor_
; CHECK:         getelementptr inbounds nuw i8, ptr @.str.shared
; CHECK-LABEL: define {{.*}}ptr @direct(
; CHECK:         ret ptr @.str.shared

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)

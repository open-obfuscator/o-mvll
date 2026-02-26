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
;     CHECK-NOT:     {{Hello1.*}}
;     CHECK-NOT:     {{Hello2.*}}

; CHECK: @0 = internal global [7 x i8] zeroinitializer
; CHECK: @1 = internal unnamed_addr global i1 false
; CHECK: @2 = internal global [7 x i8] zeroinitializer
; CHECK: @3 = internal unnamed_addr global i1 false
; CHECK: @[[NEWCA:.*]] = internal unnamed_addr constant [2 x ptr] [ptr @[[NEW1:.*]], ptr @[[NEW2:.*]]]

@.str.1 = private constant [7 x i8] c"Hello1\00", align 1
@.str.2 = private constant [7 x i8] c"Hello2\00", align 1
@__const.array = private constant [2 x ptr] [ptr @.str.1, ptr @.str.2], align 8

define void @copy(ptr %p) {
; CHECK-LABEL: define {{.*}}void @copy(
; CHECK-SAME: ptr {{.*}} [[PTR:%.*]]) {{.*}} {
; CHECK-NEXT:    [[ASM:%.*]] = tail call i64 asm sideeffect "", "=r,0"(i64 {{.*}})
; CHECK-NEXT:    [[FLAG:%.*]] = load i1, ptr @1, align 1
; CHECK-NEXT:    br i1 [[FLAG]], label %[[OMVLL_DECODE_WRAP_EXIT:.*]], label %[[BB:.*]]
; CHECK:       [[BB]]:
; CHECK-NEXT:    tail call fastcc void @__omvll_decode(i64 [[ASM]], i32 7)
; CHECK-NEXT:    store i1 true, ptr @1, align 1
; CHECK-NEXT:    br label %[[OMVLL_DECODE_WRAP_EXIT:.*]]
; CHECK:       [[OMVLL_DECODE_WRAP_EXIT]]:
; CHECK-NEXT:    [[ASM2:%.*]] = tail call i64 asm sideeffect "", "=r,0"(i64 {{.*}})
; CHECK-NEXT:    [[FLAG2:%.*]] = load i1, ptr @3, align 1
; CHECK-NEXT:    br i1 [[FLAG2]], label %[[OMVLL_DECODE_WRAP_EXIT2:.*]], label %[[BB2:.*]]
; CHECK:       [[BB2]]:
; CHECK-NEXT:    tail call fastcc void @__omvll_decode.1(i64 [[ASM2]], i32 7)
; CHECK-NEXT:    store i1 true, ptr @3, align 1
; CHECK-NEXT:    br label %[[OMVLL_DECODE_WRAP_EXIT2:.*]]
; CHECK:       [[OMVLL_DECODE_WRAP_EXIT2]]:
; CHECK-NEXT:    tail call void @llvm.memcpy.p0.p0.i64(ptr {{.*}} [[PTR:%.*]], ptr {{.*}} @[[NEWCA:.*]], i64 16, i1 false)
; CHECK-NEXT:    ret void
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %p, ptr align 8 @__const.array, i64 16, i1 false)
  ret void
}

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)

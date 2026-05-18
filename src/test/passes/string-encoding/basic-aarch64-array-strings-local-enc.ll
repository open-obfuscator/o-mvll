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
; CHECK-NEXT:    [[ALLOCA2:%.*]] = alloca i64, align 8
; CHECK-NEXT:    [[ALLOCA1:%.*]] = alloca i64, align 8
; CHECK-NEXT:    [[ASM:%.*]] = tail call i64 asm sideeffect "", "=r,0"(i64 {{.*}})
; CHECK-NEXT:    [[FLAG:%.*]] = load i1, ptr @1, align 1
; CHECK-NEXT:    br i1 [[FLAG]], label %[[EXIT1:.*]], label %[[BB1:.*]]
; CHECK:       [[BB1]]:
; CHECK-NEXT:    call void @llvm.lifetime.start.p0(i64 8, ptr nonnull [[ALLOCA1]])
; CHECK-NEXT:    store i64 [[ASM]], ptr [[ALLOCA1]], align 8
; CHECK-NEXT:    br label %[[LOOP1:.*]]
; CHECK:       [[LOOP1]]:
; CHECK-NEXT:    [[IDX1:%.*]] = phi i64 [ 0, %[[BB1]] ], [ [[IDX1_NEXT:%.*]], %[[LOOP1]] ]
; CHECK-NEXT:    [[SRC1_GEP:%.*]] = getelementptr inbounds i8, ptr @{{.*}}, i64 [[IDX1]]
; CHECK-NEXT:    [[SRC1_BYTE:%.*]] = load i8, ptr [[SRC1_GEP]], align 1
; CHECK-NEXT:    [[KEY1_GEP:%.*]] = getelementptr inbounds i8, ptr [[ALLOCA1]], i64 [[IDX1]]
; CHECK-NEXT:    [[KEY1_BYTE:%.*]] = load i8, ptr [[KEY1_GEP]], align 1
; CHECK-NEXT:    [[XOR1:%.*]] = xor i8 [[KEY1_BYTE]], [[SRC1_BYTE]]
; CHECK-NEXT:    [[DST1_GEP:%.*]] = getelementptr inbounds i8, ptr @0, i64 [[IDX1]]
; CHECK-NEXT:    store i8 [[XOR1]], ptr [[DST1_GEP]], align 1
; CHECK-NEXT:    [[IDX1_NEXT]] = add nuw nsw i64 [[IDX1]], 1
; CHECK-NEXT:    [[DONE1:%.*]] = icmp eq i64 [[IDX1_NEXT]], 7
; CHECK-NEXT:    br i1 [[DONE1]], label %[[LOOP1_EXIT:.*]], label %[[LOOP1]]
; CHECK:       [[LOOP1_EXIT]]:
; CHECK-NEXT:    call void @llvm.lifetime.end.p0(i64 8, ptr nonnull [[ALLOCA1]])
; CHECK-NEXT:    store i1 true, ptr @1, align 1
; CHECK-NEXT:    br label %[[EXIT1]]
; CHECK:       [[EXIT1]]:
; CHECK-NEXT:    [[ASM2:%.*]] = tail call i64 asm sideeffect "", "=r,0"(i64 {{.*}})
; CHECK-NEXT:    [[FLAG2:%.*]] = load i1, ptr @3, align 1
; CHECK-NEXT:    br i1 [[FLAG2]], label %[[EXIT2:.*]], label %[[BB2:.*]]
; CHECK:       [[BB2]]:
; CHECK-NEXT:    call void @llvm.lifetime.start.p0(i64 8, ptr nonnull [[ALLOCA2]])
; CHECK-NEXT:    store i64 [[ASM2]], ptr [[ALLOCA2]], align 8
; CHECK-NEXT:    br label %[[LOOP2:.*]]
; CHECK:       [[LOOP2]]:
; CHECK-NEXT:    [[IDX2:%.*]] = phi i64 [ 0, %[[BB2]] ], [ [[IDX2_NEXT:%.*]], %[[LOOP2]] ]
; CHECK-NEXT:    [[SRC2_GEP:%.*]] = getelementptr inbounds i8, ptr @{{.*}}, i64 [[IDX2]]
; CHECK-NEXT:    [[SRC2_BYTE:%.*]] = load i8, ptr [[SRC2_GEP]], align 1
; CHECK-NEXT:    [[KEY2_GEP:%.*]] = getelementptr inbounds i8, ptr [[ALLOCA2]], i64 [[IDX2]]
; CHECK-NEXT:    [[KEY2_BYTE:%.*]] = load i8, ptr [[KEY2_GEP]], align 1
; CHECK-NEXT:    [[XOR2:%.*]] = xor i8 [[KEY2_BYTE]], [[SRC2_BYTE]]
; CHECK-NEXT:    [[DST2_GEP:%.*]] = getelementptr inbounds i8, ptr @2, i64 [[IDX2]]
; CHECK-NEXT:    store i8 [[XOR2]], ptr [[DST2_GEP]], align 1
; CHECK-NEXT:    [[IDX2_NEXT]] = add nuw nsw i64 [[IDX2]], 1
; CHECK-NEXT:    [[DONE2:%.*]] = icmp eq i64 [[IDX2_NEXT]], 7
; CHECK-NEXT:    br i1 [[DONE2]], label %[[LOOP2_EXIT:.*]], label %[[LOOP2]]
; CHECK:       [[LOOP2_EXIT]]:
; CHECK-NEXT:    call void @llvm.lifetime.end.p0(i64 8, ptr nonnull [[ALLOCA2]])
; CHECK-NEXT:    store i1 true, ptr @3, align 1
; CHECK-NEXT:    br label %[[EXIT2]]
; CHECK:       [[EXIT2]]:
; CHECK-NEXT:    tail call void @llvm.memcpy.p0.p0.i64(ptr {{.*}} [[PTR]], ptr {{.*}} @[[NEWCA]], i64 16, i1 false)
; CHECK-NEXT:    ret void
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %p, ptr align 8 @__const.array, i64 16, i1 false)
  ret void
}

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)

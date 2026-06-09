;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && android_abi

;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target arm64-apple-ios26.0.0  -O1 -S -emit-llvm %s -o - | FileCheck %s
;
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target aarch64-linux-android  -O1 -S -emit-llvm %s -o - | FileCheck %s
;
;     CHECK-NOT:     {{.*Hello.*}}

@.hello = private constant [6 x i8] c"Hello\00"
; CHECK: @str = {{.*}} global ptr @0
@str = global ptr @.hello

define i32 @main() {
; CHECK-LABEL: define {{.*}}i32 @main() {{.*}} {
; CHECK-NEXT:    [[ALLOCA:%.*]] = alloca i64, align 8
; CHECK-NEXT:    [[ASM:%.*]] = tail call i64 asm sideeffect "", "=r,0"(i64 {{.*}})
; CHECK-NEXT:    [[FLAG:%.*]] = load i1, ptr @1, align 1
; CHECK-NEXT:    br i1 [[FLAG]], label %[[EXIT:.*]], label %[[BB:.*]]
; CHECK:       [[BB]]:
; CHECK-NEXT:    call void @llvm.lifetime.start.p0(i64 8, ptr nonnull [[ALLOCA]])
; CHECK-NEXT:    store i64 [[ASM]], ptr [[ALLOCA]], align 8
; CHECK-NEXT:    br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK-NEXT:    [[IDX:%.*]] = phi i64 [ 0, %[[BB]] ], [ [[IDX_NEXT:%.*]], %[[LOOP]] ]
; CHECK-NEXT:    [[SRC_GEP:%.*]] = getelementptr inbounds i8, ptr @{{.*}}, i64 [[IDX]]
; CHECK-NEXT:    [[SRC_BYTE:%.*]] = load i8, ptr [[SRC_GEP]], align 1
; CHECK-NEXT:    [[KEY_GEP:%.*]] = getelementptr inbounds i8, ptr [[ALLOCA]], i64 [[IDX]]
; CHECK-NEXT:    [[KEY_BYTE:%.*]] = load i8, ptr [[KEY_GEP]], align 1
; CHECK-NEXT:    [[XOR:%.*]] = xor i8 [[KEY_BYTE]], [[SRC_BYTE]]
; CHECK-NEXT:    [[DST_GEP:%.*]] = getelementptr inbounds i8, ptr @0, i64 [[IDX]]
; CHECK-NEXT:    store i8 [[XOR]], ptr [[DST_GEP]], align 1
; CHECK-NEXT:    [[IDX_NEXT]] = add nuw nsw i64 [[IDX]], 1
; CHECK-NEXT:    [[DONE:%.*]] = icmp eq i64 [[IDX_NEXT]], 6
; CHECK-NEXT:    br i1 [[DONE]], label %[[LOOP_EXIT:.*]], label %[[LOOP]]
; CHECK:       [[LOOP_EXIT]]:
; CHECK-NEXT:    call void @llvm.lifetime.end.p0(i64 8, ptr nonnull [[ALLOCA]])
; CHECK-NEXT:    store i1 true, ptr @1, align 1
; CHECK-NEXT:    br label %[[EXIT]]
; CHECK:       [[EXIT]]:
; CHECK-NEXT:    [[LOAD:%.*]] = load ptr, ptr @str, align 8
; CHECK-NEXT:    [[CALL:%.*]] = tail call i32 @puts(ptr nonnull dereferenceable(1) [[LOAD]])
; CHECK-NEXT:    ret i32 0
  %load = load ptr, ptr @str, align 8
  %call = call i32 @puts(ptr %load)
  ret i32 0
}

declare i32 @puts(ptr)

;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target

;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target arm64-apple-ios26.0.0  -O1 -S -emit-llvm %s -o - | FileCheck %s
;
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target aarch64-linux-android  -O1 -S -emit-llvm %s -o - | FileCheck %s
;
;     CHECK-NOT:     {{.*Hello, Stack.*}}

@__const.main.Hello = private constant [13 x i8] c"Hello, Stack\00", align 1

define void @test() {
; CHECK-LABEL: define void @test() {{.*}} {
; CHECK-NEXT:    [[ASM:%.*]] = tail call i64 asm sideeffect "", "=r,0"(i64 {{.*}})
; CHECK-NEXT:    [[FLAG:%.*]] = load i1, ptr @1, align 1
; CHECK-NEXT:    br i1 [[FLAG]], label %[[EXIT:.*]], label %[[BB:.*]]
; CHECK:       [[BB]]:
; CHECK-NEXT:    tail call fastcc void @__omvll_decode(i64 [[ASM]], i32 13)
; CHECK-NEXT:    store i1 true, ptr @1, align 1
; CHECK-NEXT:    br label %[[EXIT:.*]]
; CHECK:       [[EXIT]]:
; CHECK-NEXT:    [[CALL:%.*]] = tail call i32 @puts(ptr nonnull dereferenceable(1) @0)
; CHECK-NEXT:    ret void
  %puts = call i32 @puts(ptr @__const.main.Hello)
  ret void
}

declare i32 @puts(ptr)

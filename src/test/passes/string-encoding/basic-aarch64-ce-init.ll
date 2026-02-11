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
;     CHECK-NOT:     {{.*Hello.*}}

@.hello = private constant [6 x i8] c"Hello\00"
; CHECK: @str = {{.*}} global ptr @0
@str = global ptr @.hello

define i32 @main() {
; CHECK-LABEL: define {{.*}}i32 @main() {{.*}} {
; CHECK-NEXT:    [[ASM:%.*]] = tail call i64 asm sideeffect "", "=r,0"(i64 {{.*}})
; CHECK-NEXT:    [[FLAG:%.*]] = load i1, ptr @1, align 1
; CHECK-NEXT:    br i1 [[FLAG]], label %[[EXIT:.*]], label %[[BB:.*]]
; CHECK:       [[BB]]:
; CHECK-NEXT:    tail call fastcc void @__omvll_decode(i64 [[ASM]], i32 6)
; CHECK-NEXT:    store i1 true, ptr @1, align 1
; CHECK-NEXT:    br label %[[EXIT:.*]]
; CHECK:       [[EXIT]]:
; CHECK-NEXT:    [[LOAD:%.*]] = load ptr, ptr @str, align 8
; CHECK-NEXT:    [[CALL:%.*]] = tail call i32 @puts(ptr nonnull dereferenceable(1) [[LOAD]])
; CHECK-NEXT:    ret i32 0
  %load = load ptr, ptr @str, align 8
  %call = call i32 @puts(ptr %load)
  ret i32 0
}

declare i32 @puts(ptr)

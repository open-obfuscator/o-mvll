;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; RUN: env OMVLL_CONFIG=%S/config_all.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK %s

; A simple SESE region to outline.
define i32 @function_outline(i32 %x, i32 %y) {
; CHECK-LABEL: define i32 @function_outline(
; CHECK-SAME: i32 [[X:%.*]], i32 [[Y:%.*]])
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[USE_LOC:%.*]] = alloca i32, align 4
; CHECK-NEXT:    [[SUM:%.*]] = add i32 [[X]], [[Y]]
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i32 [[X]], [[Y]]
; CHECK-NEXT:    [[RET_REG2MEM:%.*]] = alloca i32, align 4
; CHECK-NEXT:    store i32 [[SUM]], ptr [[RET_REG2MEM]], align 4
; CHECK-NEXT:    br i1 [[CMP]], label %[[CODEREPL:.*]], label %[[EXIT:.*]]
; CHECK:       [[CODEREPL]]:
; CHECK-NEXT:    call void @function_outline.bb(i32 [[SUM]], ptr [[USE_LOC]])
; CHECK-NEXT:    [[USE_RELOAD:%.*]] = load i32, ptr [[USE_LOC]], align 4
; CHECK-NEXT:    store i32 [[USE_RELOAD]], ptr [[RET_REG2MEM]], align 4
; CHECK-NEXT:    br label %[[EXIT:.*]]
; CHECK:       [[EXIT]]:
; CHECK-NEXT:    [[RET_RELOAD:%.*]] = load i32, ptr [[RET_REG2MEM]], align 4
; CHECK-NEXT:    ret i32 [[RET_RELOAD]]
entry:
  %sum = add i32 %x, %y
  %cmp = icmp slt i32 %x, %y
  br i1 %cmp, label %bb, label %exit

bb:
  %mul = mul i32 %sum, 3
  %use = call i32 @use(i32 %mul)
  br label %exit

exit:
  %ret = phi i32 [ %use, %bb ], [ %sum, %entry ]
  ret i32 %ret
}

declare i32 @use(i32)

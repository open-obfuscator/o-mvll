;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; RUN: env OMVLL_CONFIG=%S/config_all.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK %s

define i32 @dup(i32 %arg) {
; CHECK-LABEL: define i32 @dup(
; CHECK-SAME: i32 [[ARG:%.*]])
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[CALL:%.*]] = call i64 @lrand48()
; CHECK-NEXT:    [[AND:%.*]] = and i64 [[CALL]], 1
; CHECK-NEXT:    [[CMP:%.*]] = icmp ne i64 [[AND]], 0
; CHECK-NEXT:    br i1 [[CMP]], label %[[ENTRY_SPLIT_CLONE:.*]], label %[[ENTRY_SPLIT:.*]]
; CHECK:       [[ENTRY_SPLIT]]:
; CHECK-NEXT:    br label %[[BB:.*]]
; CHECK:       [[BB]]:
; CHECK-NEXT:    [[CALL1:%.*]] = call i64 @lrand48()
; CHECK-NEXT:    [[AND1:%.*]] = and i64 [[CALL1]], 1
; CHECK-NEXT:    [[CMP1:%.*]] = icmp ne i64 [[AND1]], 0
; CHECK-NEXT:    br i1 [[CMP1]], label %[[BB_SPLIT_CLONE:.*]], label %[[BB_SPLIT:.*]]
; CHECK:       [[BB_SPLIT]]:
; CHECK-NEXT:    [[X:%.*]] = add i32 [[ARG]], 2
; CHECK-NEXT:    br label %[[EXIT:.*]]
; CHECK:       [[EXIT]]:
; CHECK-NEXT:    [[X1:%.*]] = phi i32 [ [[X_CLONE:%.*]], %[[BB_SPLIT_CLONE]] ], [ [[X:%.*]], %[[BB_SPLIT]] ]
; CHECK-NEXT:    [[CALL2:%.*]] = call i64 @lrand48()
; CHECK-NEXT:    [[AND2:%.*]] = and i64 [[CALL2]], 1
; CHECK-NEXT:    [[CMP2:%.*]] = icmp ne i64 [[AND2]], 0
; CHECK-NEXT:    br i1 [[CMP2]], label %[[EXIT_SPLIT_CLONE:.*]], label %[[EXIT_SPLIT:.*]]
; CHECK:       [[EXIT_SPLIT]]:
; CHECK-NEXT:    [[Y:%.*]] = add i32 [[X1]], 1
; CHECK-NEXT:    ret i32 [[Y]]
; CHECK:       [[ENTRY_SPLIT_CLONE]]:
; CHECK-NEXT:    br label %[[BB:.*]]
; CHECK:       [[BB_SPLIT_CLONE]]:
; CHECK-NEXT:    [[X_CLONE:%.*]] = add i32 [[ARG]], 2
; CHECK-NEXT:    br label %[[EXIT:.*]]
; CHECK:       [[EXIT_SPLIT_CLONE]]:
; CHECK-NEXT:    [[Y_CLONE:%.*]] = add i32 [[X1]], 1
; CHECK-NEXT:    ret i32 [[Y_CLONE]]
;
entry:
  br label %bb

bb:
  %x = add i32 %arg, 2
  br label %exit

exit:
  %y = add i32 %x, 1
  ret i32 %y
}

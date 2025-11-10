;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; RUN: env OMVLL_CONFIG=%S/config_all.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK,CHECK-O0 %s

; RUN: env OMVLL_CONFIG=%S/config_all.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O1 -S -emit-llvm %s -o - | FileCheck --check-prefixes=CHECK,CHECK-O1 %s

; CHECK: @[[ICALL_SHARES1:.*]] = internal constant [2 x i64] [i64 {{.*}}, i64 {{.*}}]
; CHECK: @[[ICALL_SHARES2:.*]] = internal constant [2 x i64] [i64 add (i64 ptrtoint (ptr @foo to i64), i64 {{.*}}), i64 add (i64 ptrtoint (ptr @qux to i64), i64 {{.*}})]

define void @simple_calls() {
; CHECK-LABEL:  define void @simple_calls(
; CHECK-LABEL:  entry
; CHECK-O0:       [[LD1:%.*]] = load volatile i64, ptr @[[ICALL_SHARES1]], align 8
; CHECK-O0-NEXT:  [[LD2:%.*]] = load volatile i64, ptr @[[ICALL_SHARES2]], align 8
; CHECK-O0-NEXT:  [[SUB:%.*]] = sub i64 [[LD2]], [[LD1]]
; CHECK-O0-NEXT:  [[INT2PTR:%.*]] = inttoptr i64 [[SUB]] to ptr
; CHECK-O0-NEXT:  call void [[INT2PTR]]()
; CHECK-O0-NEXT:  [[LD3:%.*]] = load volatile i64, ptr getelementptr inbounds ([2 x i64], ptr @[[ICALL_SHARES1]], i64 0, i64 1), align 8
; CHECK-O0-NEXT:  [[LD4:%.*]] = load volatile i64, ptr getelementptr inbounds ([2 x i64], ptr @[[ICALL_SHARES2]], i64 0, i64 1), align 8
; CHECK-O0-NEXT:  [[SUB2:%.*]] = sub i64 [[LD4]], [[LD3]]
; CHECK-O0-NEXT:  [[INT2PTR2:%.*]] = inttoptr i64 [[SUB2]] to ptr
; CHECK-O0-NEXT:  call void [[INT2PTR2]]()
; CHECK-O1:       [[LD1_O1:%.*]] = load volatile i64, ptr @[[ICALL_SHARES1]], align 8
; CHECK-O1-NEXT:  [[LD2_O1:%.*]] = load volatile i64, ptr @[[ICALL_SHARES2]], align 8
; CHECK-O1-NEXT:  [[SUB_O1:%.*]] = sub i64 [[LD2_O1]], [[LD1_O1]]
; CHECK-O1-NEXT:  [[INT2PTR_O1:%.*]] = inttoptr i64 [[SUB_O1]] to ptr
; CHECK-O1-NEXT:  tail call void [[INT2PTR_O1]]()
entry:
  call void @foo()
  call void @qux()
  ret void
}

declare void @foo()
declare void @qux()

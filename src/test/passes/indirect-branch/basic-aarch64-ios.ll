;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; RUN: env OMVLL_CONFIG=%S/config_all.py clang++ -fpass-plugin=%libOMVLL \
; RUN:         -target arm64-apple-ios17.5.0 -O0 -S -emit-llvm %s -o - | FileCheck %s

; CHECK: @[[INDIRECT_BLOCKADDR:[a-zA-Z0-9_$"\\.-]+]] = private constant [3 x ptr] {{.*}}, section "__DATA,__const"
; CHECK: @[[INDIRECT_BLOCKADDR2:[a-zA-Z0-9_$"\\.-]+]] = private constant [4 x ptr] {{.*}}, section "__DATA,__const"

define i32 @simple_branch(i32 %arg) {
; CHECK-LABEL:  define i32 @simple_branch(
; CHECK-LABEL:  entry
; CHECK:          [[LD:%.*]] = load ptr, ptr {{.*}}, align 8
; CHECK-NEXT:     indirectbr ptr [[LD:%.*]], [label %if]
entry:
  %add = add i32 %arg, 1
  br label %if

if:
; CHECK:          [[SEL:%.*]] = select i1 %cmp, ptr {{.*}}, ptr {{.*}}
; CHECK-NEXT:     [[LD1:%.*]] = load ptr, ptr [[SEL:%.*]], align 8
; CHECK-NEXT:     indirectbr ptr [[LD1:%.*]], [label %true, label %false]
  %cmp = icmp eq i32 %arg, 0
  br i1 %cmp, label %true, label %false

true:
  ret i32 %add

false:
  ret i32 1
}

define i32 @simple_switch(i32 %arg) {
; CHECK-LABEL:  define i32 @simple_switch(
; CHECK-LABEL:  entry
; CHECK:          [[COND:%.*]] = icmp eq i32 %arg, 1
; CHECK-NEXT:     [[SEL1:%.*]] = select i1 [[COND:%.*]], ptr {{.*}}, ptr {{.*}}
; CHECK-NEXT:     [[COND1:%.*]] = icmp eq i32 %arg, 2
; CHECK-NEXT:     [[SEL2:%.*]] = select i1 [[COND1:%.*]], ptr {{.*}}, ptr [[SEL1:%.*]]
; CHECK-NEXT:     [[COND2:%.*]] = icmp eq i32 %arg, 3
; CHECK-NEXT:     [[SEL3:%.*]] = select i1 [[COND2:%.*]], ptr {{.*}}, ptr [[SEL2:%.*]]
; CHECK-NEXT:     [[LD2:%.*]] = load ptr, ptr [[SEL3:%.*]], align 8
; CHECK-NEXT:     indirectbr ptr [[LD2:%.*]], [label %default, label %case1, label %case2, label %case3]
entry:
  %add = add i32 %arg, 1
  switch i32 %arg, label %default [
    i32 1, label %case1
    i32 2, label %case2
    i32 3, label %case3
  ]

case1:
  ret i32 0

case2:
  ret i32 1

case3:
  ret i32 2

default:
  ret i32 %add
}

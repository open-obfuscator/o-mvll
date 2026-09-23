;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; Regression test for the OpaqueFieldAccess struct-read path.
;
; The pass replaces every user of the field GEP with a new byte-offset GEP via
; replaceAllUsesWith. That replacement must dominate ALL users of the original
; GEP, not just the load being processed. 

; RUN: env OMVLL_CONFIG=%S/config.py clang -target aarch64-linux-android -fpass-plugin=%libOMVLL -O0 -S -emit-llvm %s -o - | FileCheck %s
; RUN: env OMVLL_CONFIG=%S/config.py clang -target arm64-apple-ios       -fpass-plugin=%libOMVLL -O0 -S -emit-llvm %s -o - | FileCheck %s

%struct.Point = type { i32, i32 }

; Reads field `y` (offset 4) twice: once as an address (ptrtoint) and once as a
; value (load). Both refer to the same GEP, so it has two users.
define i64 @read_twice(ptr %p) {
; CHECK-LABEL: @read_twice
; The obfuscated byte offset and byte-level GEP are inserted at the original
; GEP, so they dominate both users below.
; CHECK: %[[OFF:.*]] = add i32 0, 4
; CHECK: %[[NEWGEP:.*]] = getelementptr inbounds i8, ptr %p, i32 %[[OFF]]
; The earlier user (ptrtoint) must reference the new GEP defined above it.
; CHECK: ptrtoint ptr %[[NEWGEP]] to i64
; CHECK: load i32, ptr %[[NEWGEP]]
entry:
  %gep = getelementptr inbounds %struct.Point, ptr %p, i32 0, i32 1
  %addr = ptrtoint ptr %gep to i64
  %val = load i32, ptr %gep, align 4
  %ext = sext i32 %val to i64
  %sum = add i64 %addr, %ext
  ret i64 %sum
}

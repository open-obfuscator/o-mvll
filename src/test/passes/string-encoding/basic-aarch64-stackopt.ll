;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target

;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target arm64-apple-ios17.5.0 -S -emit-llvm -O0 -c %s -o - | FileCheck %s
;
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target aarch64-linux-android -S -emit-llvm -O0 -c %s -o - | FileCheck %s
;
;     CHECK-NOT:     {{.*Hello, Stack.*}}

@__const.main.Hello = private constant [13 x i8] c"Hello, Stack\00", align 1

define i32 @main() {
; CHECK-LABEL: @main(
; CHECK:         %5 = getelementptr inbounds [13 x i8], ptr @0, i64 0, i64 0
; CHECK-NEXT:    %6 = load i1, ptr @1, align 1
; CHECK-NEXT:    %7 = icmp eq i1 %6, false
; CHECK-NEXT:    br i1 %7, label %8, label %__omvll_decode_wrap.exit
; CHECK:       8:
; CHECK-NEXT:    call void @__omvll_decode(ptr %5, ptr @__const.main.Hello, i64 %3, i32 %4)
; CHECK-NEXT:    store i1 true, ptr @1, align 1
; CHECK-NEXT:    br label %__omvll_decode_wrap.exit
; CHECK:       __omvll_decode_wrap.exit:
; CHECK-NEXT:    call void @llvm.memcpy.p0.p0.i64(ptr align 1 %buffer, ptr align 1 %5, i64 13, i1 false)
  %buffer = alloca [13 x i8], align 1
  call void @llvm.lifetime.start.p0(i64 13, ptr %buffer)
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %buffer, ptr align 1 @__const.main.Hello, i64 13, i1 false)
  %puts = call i32 @puts(ptr %buffer)
  call void @llvm.lifetime.end.p0(i64 13, ptr %buffer)
  ret i32 0
}

declare void @llvm.lifetime.start.p0(i64, ptr)
declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)
declare i32 @puts(ptr)
declare void @llvm.lifetime.end.p0(i64, ptr)

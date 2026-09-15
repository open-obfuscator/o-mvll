;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; A Swift `static let` string also stores its address in the property storage, a
; constant global the loader materialises. Local encoding redirects reads by
; rewriting the instruction operand naming the literal, and an initializer has no
; such operand, so that literal has to be left in the clear. The decision is per
; literal: @.str.enc, reached only from the getter, is still encoded.
;
; Also covers the getter's memory(none): the stub stores to @0 and @1, so leaving
; the attribute on lets later passes fold those stores away and the string
; decodes to "".
;
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target arm64-apple-ios17.5.0 -O1 -S -emit-llvm %s -o - | FileCheck %s
;
;     CHECK-NOT: {{.*EncodedString.*}}

@.str.kept = private unnamed_addr constant [22 x i8] c"Hello, KeptInTheClear\00"
; CHECK: @.str.kept = {{.*}}c"Hello, KeptInTheClear\00"

@.str.enc = private unnamed_addr constant [21 x i8] c"Hello, EncodedString\00"

@storage = constant { i64, ptr } {
    i64 -3458764513820540907,
    ptr inttoptr (i64 add (i64 ptrtoint (ptr @.str.kept to i64), i64 9223372036854775776) to ptr) }, align 8
; CHECK: @storage = {{.*}}ptrtoint (ptr @.str.kept to i64)

; CHECK: @0 = internal global [21 x i8] zeroinitializer
; CHECK: @1 = internal unnamed_addr global i1 false

define swiftcc { ptr, ptr } @getter(ptr readnone swiftself captures(none) %0) #0 {
; CHECK:       Function Attrs:
; CHECK-NOT:     memory(
; CHECK:       define swiftcc { ptr, ptr } @getter
; CHECK:         icmp eq i64 {{.*}}, 21
; CHECK:         store i1 true, ptr @1
; CHECK:         {{.*}} = or i64 sub (i64 ptrtoint (ptr @.str.kept to i64), i64 32), -9223372036854775808
; CHECK:         {{.*}} = or i64 sub (i64 ptrtoint (ptr @0 to i64), i64 32), -9223372036854775808
entry:
  %1 = or i64 sub (i64 ptrtoint (ptr @.str.kept to i64), i64 32), -9223372036854775808
  %2 = inttoptr i64 %1 to ptr
  %3 = or i64 sub (i64 ptrtoint (ptr @.str.enc to i64), i64 32), -9223372036854775808
  %4 = inttoptr i64 %3 to ptr
  %5 = insertvalue { ptr, ptr } undef, ptr %2, 0
  %6 = insertvalue { ptr, ptr } %5, ptr %4, 1
  ret { ptr, ptr } %6
}

; CHECK: attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn "

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }

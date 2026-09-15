;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target && apple_abi

; A Swift `static let` string also stores its address in the property storage, a
; constant global the loader materialises. Local decoding rewrites the
; instruction operand naming the literal, and an initializer has no such operand,
; so such a literal is encoded globally instead: that decodes it in place, which
; leaves the stored address valid. The choice is per literal -- @.str.local,
; reached only from the getter, is still encoded locally.
;
; Also covers the getter's memory(none): the local stub stores to @0 and @1, so
; leaving the attribute on lets later passes fold those stores away and the
; string decodes to "".
;
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target arm64-apple-ios17.5.0 -O1 -S -emit-llvm %s -o - | FileCheck %s
;
; Neither literal may survive in the clear.
;     CHECK-NOT: {{.*GlobalEncoded.*}}
;     CHECK-NOT: {{.*LocalEncoded.*}}

; Encoded in place, so it loses `constant` and gains a load-time constructor.
@.str.glob = private unnamed_addr constant [21 x i8] c"Hello, GlobalEncoded\00"
; CHECK: @.str.glob = private unnamed_addr global [21 x i8] c"

@.str.local = private unnamed_addr constant [20 x i8] c"Hello, LocalEncoded\00"

; The property storage keeps naming the literal: it is decoded where it lies.
@storage = constant { i64, ptr } {
    i64 -3458764513820540907,
    ptr inttoptr (i64 add (i64 ptrtoint (ptr @.str.glob to i64), i64 9223372036854775776) to ptr) }, align 8
; CHECK: @storage = {{.*}}ptrtoint (ptr @.str.glob to i64)

; One local buffer, for @.str.local only.
; CHECK: @0 = internal global [20 x i8] zeroinitializer
; CHECK: @1 = internal unnamed_addr global i1 false
; CHECK: @llvm.global_ctors = {{.*}}@__omvll_ctor_

define swiftcc { ptr, ptr } @getter(ptr readnone swiftself captures(none) %0) #0 {
; CHECK:       Function Attrs:
; CHECK-NOT:     memory(
; CHECK:       define swiftcc { ptr, ptr } @getter
; CHECK:         icmp eq i64 {{.*}}, 20
; CHECK:         store i1 true, ptr @1
; CHECK:         {{.*}} = or i64 sub (i64 ptrtoint (ptr @.str.glob to i64), i64 32), -9223372036854775808
; CHECK:         {{.*}} = or i64 sub (i64 ptrtoint (ptr @0 to i64), i64 32), -9223372036854775808
entry:
  %1 = or i64 sub (i64 ptrtoint (ptr @.str.glob to i64), i64 32), -9223372036854775808
  %2 = inttoptr i64 %1 to ptr
  %3 = or i64 sub (i64 ptrtoint (ptr @.str.local to i64), i64 32), -9223372036854775808
  %4 = inttoptr i64 %3 to ptr
  %5 = insertvalue { ptr, ptr } undef, ptr %2, 0
  %6 = insertvalue { ptr, ptr } %5, ptr %4, 1
  ret { ptr, ptr } %6
}

; CHECK: attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn "

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }

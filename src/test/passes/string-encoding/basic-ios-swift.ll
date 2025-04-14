;
; This file is distributed under the Apache License v2.0. See LICENSE for details.
;

; REQUIRES: aarch64-registered-target

; The 'replace' configuration encodes the string and adds logic that decodes it at load-time:
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target arm64-apple-ios17.5.0 -O1 -c %s -o - | strings | FileCheck %s
;
;     CHECK-NOT:     {{.*Hello, Swift.*}}

%swift.bridge = type opaque

declare swiftcc { i64, ptr, i1, ptr } @"foo"(i64 %0, ptr %1)
declare void @baz(ptr)

@.str.4 = private constant [13 x i8] c"Hello, Swift\00"

define { i64, ptr, i1, ptr } @test_function() {
  %1 = call swiftcc { i64, ptr, i1, ptr } @"foo"(i64 -3458764513820540911, ptr inttoptr (i64 sub (i64 ptrtoint ([13 x i8]* @.str.4 to i64), i64 32) to ptr))
  ret { i64, ptr, i1, ptr } %1
}

define void @test_function2() {
  call void @baz(ptr @.str.4)
  ret void
}

; REQUIRES: aarch64-registered-target

; The 'replace' configuration encodes the string and adds logic that decodes it at load-time:
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target arm64-apple-ios  -fno-legacy-pass-manager -O1 -c %s -o - | strings | FileCheck %s
;
;     CHECK-NOT:     {{.*Hello, Swift.*}}

%swift.bridge = type opaque

declare swiftcc { i64, %swift.bridge*, i1, %swift.bridge* } @"foo"(i64 %0, %swift.bridge* %1)

@.str.4 = private constant [13 x i8] c"Hello, Swift\00"

define { i64, %swift.bridge*, i1, %swift.bridge* } @test_function() {
  %1 = call swiftcc { i64, %swift.bridge*, i1, %swift.bridge* } @"foo"(i64 -3458764513820540911, %swift.bridge* inttoptr (i64 or (i64 sub (i64 ptrtoint ([13 x i8]* @.str.4 to i64), i64 32), i64 -9223372036854775808) to %swift.bridge*))
  ret { i64, %swift.bridge*, i1, %swift.bridge* } %1
}

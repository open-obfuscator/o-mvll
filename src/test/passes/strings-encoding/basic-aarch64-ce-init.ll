; REQUIRES: aarch64-registered-target

;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target arm64-apple-ios        -O1 -c %s -o - | strings | FileCheck %s
;
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target aarch64-linux-android  -O1 -c %s -o - | strings | FileCheck %s
;
;     CHECK-NOT:     {{.*Hello.*}}

@.hello = private constant [6 x i8] c"Hello\00"
@str = global ptr @.hello

define i32 @main() #0 {
  %load = load ptr, ptr @str, align 8
  %call = call i32 @puts(ptr nonnull %load)
  ret i32 0
}

declare i32 @puts(ptr noundef)

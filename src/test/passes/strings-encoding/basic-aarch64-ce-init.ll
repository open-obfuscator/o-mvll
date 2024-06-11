; REQUIRES: aarch64-registered-target

;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target arm64-apple-ios  -fno-legacy-pass-manager -O1 -c %s -o - | strings | FileCheck %s
;
;     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
;     RUN:         -target aarch64-linux-android  -fno-legacy-pass-manager -O1 -c %s -o - | strings | FileCheck %s
;
;     CHECK-NOT:     {{.*Hello.*}}

@.hello = private constant [6 x i8] c"Hello\00"
@str = global i8* getelementptr inbounds ([6 x i8], [6 x i8]* @.hello, i32 0, i32 0)

define i32 @main() #0 {
  %load = load i8*, i8** @str, align 8
  %call = call i32 @puts(i8* %load)
  ret i32 0
}

declare i32 @puts(i8*)

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// REQUIRES: x86-registered-target

// RUN: env OMVLL_CONFIG=%S/report_diff.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o /dev/null -DNEEDS_OBFUSCATION | FileCheck %s
// RUN: env OMVLL_CONFIG=%S/report_diff.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o /dev/null | FileCheck --allow-empty --check-prefix=NO_REPORT %s

// Make sure the report_diff function was called if the obfuscation was applied
// CHECK: omvll::Arithmetic applied obfuscation
// CHECK: --- original
// CHECK: +++ obfuscated
// CHECK: @@
// CHECK: -
// CHECK: +

// Make sure the report_diff function was NOT called if the obfuscation was NOT applied
// NO_REPORT-NOT: omvll::Arithmetic applied obfuscation

void test(char *dst, const char *src, unsigned len) {
#if defined(NEEDS_OBFUSCATION)
  *dst = *src ^ 35;
#else
  *dst = *src;
#endif
}

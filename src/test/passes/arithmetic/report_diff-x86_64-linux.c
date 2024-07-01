// REQUIRES: x86-registered-target

// RUN: env OMVLL_CONFIG=%S/report_diff.py clang -target x86_64-pc-linux-gnu -fpass-plugin=%libOMVLL -O1 -S %s -o /dev/null | FileCheck %s

// CHECK: omvll::Arithmetic applied obfuscation
// CHECK: --- original
// CHECK: +++ obfuscated
// CHECK: @@
// CHECK: -
// CHECK: +

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

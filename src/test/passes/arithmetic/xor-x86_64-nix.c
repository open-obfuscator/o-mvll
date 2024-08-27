// REQUIRES: x86-registered-target

// RUN:                                        clang -target x86_64-apple-darwin -fno-legacy-pass-manager                         -O1 -S -emit-llvm %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_0.py clang -target x86_64-apple-darwin -fno-legacy-pass-manager -fpass-plugin=%libOMVLL -O1 -S -emit-llvm %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target x86_64-apple-darwin -fno-legacy-pass-manager -fpass-plugin=%libOMVLL -O1 -S -emit-llvm %s -o - | FileCheck --check-prefix=RX %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_3.py clang -target x86_64-apple-darwin -fno-legacy-pass-manager -fpass-plugin=%libOMVLL -O1 -S -emit-llvm %s -o - | FileCheck --check-prefix=RX %s

// RUN:                                        clang -target x86_64-pc-linux-gnu -fno-legacy-pass-manager                         -O1 -S -emit-llvm %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_0.py clang -target x86_64-pc-linux-gnu -fno-legacy-pass-manager -fpass-plugin=%libOMVLL -O1 -S -emit-llvm %s -o - | FileCheck --check-prefix=R0 %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_2.py clang -target x86_64-pc-linux-gnu -fno-legacy-pass-manager -fpass-plugin=%libOMVLL -O1 -S -emit-llvm %s -o - | FileCheck --check-prefix=RX %s
// RUN: env OMVLL_CONFIG=%S/config_rounds_3.py clang -target x86_64-pc-linux-gnu -fno-legacy-pass-manager -fpass-plugin=%libOMVLL -O1 -S -emit-llvm %s -o - | FileCheck --check-prefix=RX %s

// R0-LABEL: void @memcpy_xor
// R0:         xor {{.*}}, 35

// RX-LABEL: void @memcpy_xor
// RX-NOT:     xor {{.*}}, 35

void memcpy_xor(char *dst, const char *src, unsigned len) {
  for (unsigned i = 0; i < len; i += 1) {
    dst[i] = src[i] ^ 35;
  }
  dst[len] = '\0';
}

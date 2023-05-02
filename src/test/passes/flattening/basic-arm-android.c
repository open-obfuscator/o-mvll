// REQUIRES: arm-registered-target
// XFAIL: *

// RUN:                                   clang -target arm-linux-android -fno-legacy-pass-manager                         -O1 -fno-verbose-asm -S %s -o -
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target arm-linux-android -fno-legacy-pass-manager -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o /dev/null 2>&1 | FileCheck --check-prefix=ERR %s

// Compilation fails with flattening enabled
// ERR: <inline asm>:2:5: error: invalid instruction
// ERR:     ldr x1, #-8;
// ERR:     ^
// ERR: <inline asm>:3:5: error: invalid instruction, did you mean: b, bl, ldr, lsr?
// ERR:     blr x1;
// ERR:     ^
// ERR: <inline asm>:4:5: error: invalid instruction
// ERR:     mov x0, x1;
// ERR:     ^
// ERR: 3 errors generated.

int check_password(const char* passwd, unsigned len) {
  if (len != 5) {
    return 0;
  }
  if (passwd[0] == 'O') {
    if (passwd[1] == 'M') {
      if (passwd[2] == 'V') {
        if (passwd[3] == 'L') {
          if (passwd[4] == 'L') {
            return 1;
          }
        }
      }
    }
  }
  return 0;
}

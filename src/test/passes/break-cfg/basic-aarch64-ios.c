// REQUIRES: aarch64-registered-target
// XFAIL: host-platform-linux

// RUN:                                   clang -target arm64-apple-ios -fno-legacy-pass-manager                         -O1 -fno-verbose-asm -S %s -o - | FileCheck %s
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target arm64-apple-ios -fno-legacy-pass-manager -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o - | FileCheck --check-prefix=BREAKCFG-IOS %s

// BREAKCFG-IOS-LABEL: _check_password.1:
//                     ; adr x1, #0x10
//                     ; ldr x0, [x1, #61]
//                     ; ldr x1, #16
//                     ; blr x1
// BREAKCFG-IOS:       .byte	129
// BREAKCFG-IOS:       .byte	0
// BREAKCFG-IOS:       .byte	0
// BREAKCFG-IOS:       .byte	16
// BREAKCFG-IOS:       .byte	32
// BREAKCFG-IOS:       .byte	208
// BREAKCFG-IOS:       .byte	67
// BREAKCFG-IOS:       .byte	248  
// BREAKCFG-IOS:       .byte	129
// BREAKCFG-IOS:       .byte	0
// BREAKCFG-IOS:       .byte	0
// BREAKCFG-IOS:       .byte	88
// BREAKCFG-IOS:       .byte	32
// BREAKCFG-IOS:       .byte	0
// BREAKCFG-IOS:       .byte	63
// BREAKCFG-IOS:       .byte	214

// CHECK-LABEL: _check_password:
// BREAKCFG-IOS:       Lloh0:
// BREAKCFG-IOS:       adrp	x8, _check_password.1@PAGE
// BREAKCFG-IOS:       Lloh1:
// BREAKCFG-IOS:       add	x8, x8, _check_password.1@PAGEOFF
// BREAKCFG-IOS:       str	x8, [sp]
// BREAKCFG-IOS:       add	x8, x8, #32
// BREAKCFG-IOS:       str	x8, [sp, #8]
// BREAKCFG-IOS:       ldr	x8, [sp, #8]
// BREAKCFG-IOS:       blr	x8

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

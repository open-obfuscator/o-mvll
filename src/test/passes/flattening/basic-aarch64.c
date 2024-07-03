// REQUIRES: aarch64-registered-target
// TODO: Add CHECK lines

// RUN:                                   clang -target aarch64-linux-android                         -O1 -fno-verbose-asm -S %s -o -
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target aarch64-linux-android -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o -

// RUN:                                   clang -target arm64-apple-ios                          -O1 -fno-verbose-asm -S %s -o -
// RUN: env OMVLL_CONFIG=%S/config_all.py clang -target arm64-apple-ios  -fpass-plugin=%libOMVLL -O1 -fno-verbose-asm -S %s -o -

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

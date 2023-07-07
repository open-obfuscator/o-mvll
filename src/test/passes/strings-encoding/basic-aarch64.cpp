// REQUIRES: aarch64-registered-target
// XFAIL: host-platform-macOS && host-arch-x86

// The default object contains the file-name string:
//     RUN: clang++ -target aarch64-linux-android -fno-legacy-pass-manager -O1 -c %s -o - | strings | FileCheck --check-prefix=CHECK-DEFAULT  -DFILE_NAME=%s %s
//     RUN: clang++ -target arm64-apple-iphoneos  -fno-legacy-pass-manager -O1 -c %s -o - | strings | FileCheck --check-prefix=CHECK-DEFAULT  -DFILE_NAME=%s %s
//     CHECK-DEFAULT: [[FILE_NAME]]

// The 'remove' configuration overwrites it with the fixed REDACTED literal:
//     RUN: env OMVLL_CONFIG=%S/config_remove.py clang++ -fpass-plugin=%libOMVLL \
//     RUN:         -target aarch64-linux-android -fno-legacy-pass-manager -O1 -c %s -o - | strings | FileCheck --check-prefix=CHECK-REMOVED  -DFILE_NAME=%s %s
//
//     RUN: env OMVLL_CONFIG=%S/config_remove.py clang++ -fpass-plugin=%libOMVLL -O1 \
//     RUN:         -target arm64-apple-iphoneos  -fno-legacy-pass-manager -O1 -c %s -o - | strings | FileCheck --check-prefix=CHECK-REMOVED  -DFILE_NAME=%s %s
//
//     CHECK-REMOVED-NOT: [[FILE_NAME]]
//     CHECK-REMOVED:     REDACTED

// The 'replace' configuration encodes the string and adds logic that decodes it at load-time:
//     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
//     RUN:         -target aarch64-linux-android -fno-legacy-pass-manager -O1 -c %s -o - | strings | FileCheck --check-prefix=CHECK-REPLACED -DFILE_NAME=%s %s
//
//     RUN: env OMVLL_CONFIG=%S/config_replace.py clang++ -fpass-plugin=%libOMVLL \
//     RUN:         -target arm64-apple-iphoneos  -fno-legacy-pass-manager -O1 -c %s -o - | strings | FileCheck --check-prefix=CHECK-REPLACED -DFILE_NAME=%s %s
//
//     CHECK-REPLACED-NOT: [[FILE_NAME]]

extern void *stderr;
extern int fprintf(void * __stream, const char *__format, ...);

#define LOG_ERROR(MSG) fprintf(stderr, "Error: %s (%s:%d)\n", MSG, __FILE__, __LINE__)

bool check_code(int code) {
  if (code != 2) {
    LOG_ERROR("Wrong input");
    return false;
  }
  return true;
}

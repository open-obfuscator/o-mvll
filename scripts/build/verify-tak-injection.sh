#!/bin/bash
# Make sure TakInjection pass succeeded.
# To be run as a post-build phase script in Xcode.

[[ "$(uname)" != "Darwin" ]] && { echo "[-] macOS only." >&2; exit 255; }

TEMP_DIR=$(getconf DARWIN_USER_TEMP_DIR)
CACHE_DIR=$(getconf DARWIN_USER_CACHE_DIR)

TEMP_FILE_PATH="${TEMP_DIR}omvll_tak_injection"
CACHE_FILE_PATH="${CACHE_DIR}omvll_tak_injection"

check_and_remove() {
    local file_path=$1
    if [ -f "$file_path" ]; then
        echo "[+] TakInjection succeeded."
        rm "$file_path"
        exit 0
    fi
    return 1
}

check_and_remove "$TEMP_FILE_PATH"
check_and_remove "$CACHE_FILE_PATH"

echo "[-] Failed to execute TakInjection, please report any issue to Build38."
exit 255

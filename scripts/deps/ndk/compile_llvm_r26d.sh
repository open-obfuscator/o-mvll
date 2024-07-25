#!/usr/bin/env bash
# This script is used to compile the Android NDK r26d LLVM toolchain
set -e

host=$(uname)

NDK_VERSION=26.3.11579264

if [ "$host" == "Darwin" ]; then
    ndk_platform="darwin-x86_64"
    manifest="manifest_11349228.xml"
else
    ndk_platform="linux-x86_64"
    manifest="manifest_11349228.xml"
fi

mkdir android-llvm-toolchain-r26d-tmp && cd android-llvm-toolchain-r26d-tmp
repo init -u https://android.googlesource.com/platform/manifest -b llvm-toolchain

cp $ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/$ndk_platform/${manifest} .repo/manifests/
repo init -m ${manifest}
repo sync -c

python3 toolchain/llvm_android/build.py --skip-tests

export NDK_STAGE1=$(pwd)/out/stage1-install
export NDK_STAGE2=$(pwd)/out/stage2-install

zero_out() {
    local BIN_DIR="$1"

    local KEEP_BINARIES=("clang-17" "clang" "clang++" "clang-cpp" "clang-cl" "clang-extdef-mapping" "clang-format"
                         "clang-nvlink-wrapper" "clang-offload-bundler" "clang-offload-wrapper"
                         "git-clang-format" "hmaptool" "llvm-config" "llvm-link" "llvm-lit"
                         "llvm-tblgen" "FileCheck" "count" "not"
                         "amdgpu-arch" "clangd" "find-all-symbols" "ld.lld" "ld64.lld" "llc" "lld"
                         "lld-link" "lldb" "lldb-argdumper" "lldb-instr" "lldb-server" "lldb-vscode"
                         "lli" "modularize" "nvptx-arch" "opt" "pp-trace" "run-clang-tidy" "sancov"
                         "sanstats" "verify-uselistorder" "wasm-ld")

    for file in "$BIN_DIR"/*; do
        if [[ ! "${KEEP_BINARIES[@]}" =~ "$(basename "$file")" ]]; then
            echo "Zeroing out $file"
            echo -n > "$file"
        fi
    done
}

# Cleanup stages folder before generating final package
zero_out $NDK_STAGE1/bin
zero_out $NDK_STAGE2/bin

# Generate final package
cd .. && mkdir -p android-llvm-toolchain-r26d/out && cd android-llvm-toolchain-r26d/out
cp -r ${NDK_STAGE1} .
cp -r ${NDK_STAGE2} .
mv ./stage2-install stage2

tar czf stage1-install.tar.gz stage1-install && rm -rf stage1-install
tar czf stage2.tar.gz stage2 && rm -rf stage2
cd .. && tar czf out.tar.gz out && rm -rf out
cd .. && tar czf android-llvm-toolchain-r26d.tar.gz android-llvm-toolchain-r26d

# Clean up
rm -rf android-llvm-toolchain-r26d

# Move to the final folder
mv android-llvm-toolchain-r26d.tar.gz ./omvll-deps/

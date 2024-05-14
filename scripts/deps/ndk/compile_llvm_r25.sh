#!/usr/bin/env bash
# This script is used to compile ndkr25c
set -e

host=$(uname)

NDK_VERSION=25.1.8937393

if [ "$host" == "Darwin" ]; then
    ndk_platform="darwin-x86_64"
    manifest="manifest_8490178.xml"
else
    ndk_platform="linux-x86_64"
    manifest="manifest_9352603.xml"
fi

mkdir android-llvm-toolchain-r25c-tmp && cd android-llvm-toolchain-r25c-tmp
repo init -u https://android.googlesource.com/platform/manifest -b llvm-toolchain

cp $ANDROID_HOME/ndk/$NDK_VERSION/toolchains/llvm/prebuilt/$ndk_platform/${manifest} .repo/manifests/
repo init -m ${manifest}
repo sync -c

python3 toolchain/llvm_android/build.py --skip-tests

export NDK_STAGE1=$(pwd)/out/stage1-install
export NDK_STAGE2=$(pwd)/out/stage2-install

zero_out() {
    local BIN_DIR="$1"

    local KEEP_BINARIES=("clang-14" "clang" "clang++" "clang-cpp" "clang-cl" "clang-extdef-mapping" "clang-format"
                         "clang-nvlink-wrapper" "clang-offload-bundler" "clang-offload-wrapper"
                         "git-clang-format" "hmaptool" "llvm-config" "llvm-link" "llvm-lit"
                         "llvm-tblgen" "FileCheck" "count" "not")

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
cd .. && mkdir -p android-llvm-toolchain-r25c/out && cd android-llvm-toolchain-r25c/out
cp -r ${NDK_STAGE1} .
cp -r ${NDK_STAGE2} .
mv ./stage2-install stage2

tar czf stage1-install.tar.gz stage1-install && rm -rf stage1-install
tar czf stage2.tar.gz stage2 && rm -rf stage2
cd .. && tar czf out.tar.gz out && rm -rf out
cd .. && tar czf android-llvm-toolchain-r25c.tar.gz android-llvm-toolchain-r25c

# Clean up
rm -rf android-llvm-toolchain-r25c

#!/usr/bin/env bash
# This script is used to compile xcode llvm
set -e
LLVM_TARGET="AArch64;X86"

git clone -j8 --branch swift/release/5.7 --single-branch --depth 1 \
          https://github.com/apple/llvm-project.git

echo "LLVM XCODE Version: $(git --git-dir=llvm-project/.git describe --dirty)"

LLVM_ROOT=$(pwd)/llvm-project
BUILD_ROOT=$(pwd)/build
rm -rf ${BUILD_ROOT}
cmake -GNinja -S ${LLVM_ROOT}/llvm              \
      -B ${BUILD_ROOT}                          \
      -DCMAKE_BUILD_TYPE=Release                \
      -DCMAKE_OSX_ARCHITECTURES="arm64"         \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"      \
      -DLLVM_ENABLE_LTO=OFF                     \
      -DLLVM_ENABLE_TERMINFO=OFF                \
      -DLLVM_ENABLE_THREADS=ON                  \
      -DLLVM_USE_NEWPM=ON                       \
      -DLLVM_TARGET_ARCH=${LLVM_TARGET}         \
      -DLLVM_TARGETS_TO_BUILD=${LLVM_TARGET}    \
      -DLLVM_INCLUDE_TESTS=ON                   \
      -DLLVM_ENABLE_PROJECTS="clang;llvm"

ninja -C ${BUILD_ROOT} package

tar xzvf ${BUILD_ROOT}/LLVM-14.0.0git-Darwin.tar.gz
mv ${BUILD_ROOT}/LLVM-14.0.0git-Darwin ${BUILD_ROOT}/LLVM-14.0.0git-arm64-Darwin
tar czf ./omvll-deps/LLVM-14.0.0git-arm64-Darwin.tar.gz ${BUILD_ROOT}/LLVM-14.0.0git-arm64-Darwin

rm -rf ${BUILD_ROOT}

# Same for x86_64
cmake -GNinja -S ${LLVM_ROOT}/llvm              \
      -B ${BUILD_ROOT}                          \
      -DCMAKE_BUILD_TYPE=Release                \
      -DCMAKE_OSX_ARCHITECTURES="x86_64"        \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0"      \
      -DLLVM_ENABLE_LTO=OFF                     \
      -DLLVM_ENABLE_TERMINFO=OFF                \
      -DLLVM_ENABLE_THREADS=ON                  \
      -DLLVM_USE_NEWPM=ON                       \
      -DLLVM_TARGET_ARCH=${LLVM_TARGET}         \
      -DLLVM_TARGETS_TO_BUILD=${LLVM_TARGET}    \
      -DLLVM_INCLUDE_TESTS=ON                   \
      -DLLVM_ENABLE_PROJECTS="clang;llvm"

ninja -C ${BUILD_ROOT} package

tar xzvf ${BUILD_ROOT}/LLVM-14.0.0git-Darwin.tar.gz
mv ${BUILD_ROOT}/LLVM-14.0.0git-Darwin ${BUILD_ROOT}/LLVM-14.0.0git-x86_64-Darwin
tar czf ./omvll-deps/LLVM-14.0.0git-x86_64-Darwin.tar.gz ${BUILD_ROOT}/LLVM-14.0.0git-x86_64-Darwin
rm -rf ${BUILD_ROOT}

#!/usr/bin/sh

#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

set -ex

cd /deps

tar xzvf LLVM-21.1-arm64-Darwin.tar.gz
rm LLVM-21.1-arm64-Darwin.tar.gz

tar xzvf LLVM21-NDK29-Darwin.tar.gz
rm LLVM21-NDK29-Darwin.tar.gz

tar xzvf Python-slim.tar.gz
rm Python-slim.tar.gz

tar xzvf pybind11.tar.gz
rm pybind11.tar.gz

tar xzvf spdlog-1.10.0-Darwin.tar.gz
rm spdlog-1.10.0-Darwin.tar.gz

export OSXCROSS_TARGET_DIR=/osxcross
export OSXCROSS_SDK=${OSXCROSS_TARGET_DIR}/SDK/MacOSX26.4.sdk
export OMVLL_PYTHONPATH=/omvll/ci/distribution/Python-3.10.7/Lib

cd /o-mvll/src

mkdir -p build_xcode && cd build_xcode

export OSXCROSS_HOST="arm64-apple-darwin25.4"
export OSXCROSS_TARGET="arm64-apple-darwin25.4"

cmake -GNinja -Bxcode-arm64 -S.. \
      -DCMAKE_TOOLCHAIN_FILE=${OSXCROSS_TARGET_DIR}/toolchain.cmake \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="15.4" \
      -DOSXCROSS_HOST=${OSXCROSS_HOST} \
      -DOSXCROSS_TARGET_DIR=${OSXCROSS_TARGET_DIR} \
      -DOSXCROSS_SDK=${OSXCROSS_SDK} \
      -DOSXCROSS_TARGET=${OSXCROSS_TARGET} \
      -DCMAKE_BUILD_TYPE=Release \
      -DPYBIND11_NOPYTHON=1 \
      -DPython3_ROOT_DIR=/deps \
      -DPython3_LIBRARY=/deps/lib/libpython3.10.a \
      -DPython3_INCLUDE_DIR=/deps/include/python3.10 \
      -Dpybind11_DIR=/deps/share/cmake/pybind11 \
      -Dspdlog_DIR=/deps/lib/cmake/spdlog \
      -DLLVM_DIR=/deps/LLVM-21.1-arm64-Darwin/lib/cmake/llvm \
      -DOMVLL_ABI=Apple
ninja -C xcode-arm64

cmake -GNinja -Bndk-arm64 -S.. \
      -DCMAKE_TOOLCHAIN_FILE=${OSXCROSS_TARGET_DIR}/toolchain.cmake \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="15.4" \
      -DOSXCROSS_HOST=${OSXCROSS_HOST} \
      -DOSXCROSS_TARGET_DIR=${OSXCROSS_TARGET_DIR} \
      -DOSXCROSS_SDK=${OSXCROSS_SDK} \
      -DOSXCROSS_TARGET=${OSXCROSS_TARGET} \
      -DCMAKE_BUILD_TYPE=Release \
      -DPYBIND11_NOPYTHON=1 \
      -DPython3_ROOT_DIR=/deps \
      -DPython3_LIBRARY=/deps/lib/libpython3.10.a \
      -DPython3_INCLUDE_DIR=/deps/include/python3.10 \
      -Dpybind11_DIR=/deps/share/cmake/pybind11 \
      -Dspdlog_DIR=/deps/lib/cmake/spdlog \
      -DLLVM_DIR=/deps/LLVM21-NDK29-Darwin/lib/cmake/llvm \
      -DOMVLL_ABI=Android
ninja -C ndk-arm64

export OSXCROSS_HOST="x86_64-apple-darwin25.4"
export OSXCROSS_TARGET="x86_64-apple-darwin25.4"

cmake -GNinja -Bndk-x86_64 -S.. \
      -DCMAKE_TOOLCHAIN_FILE=${OSXCROSS_TARGET_DIR}/toolchain.cmake \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="15.4" \
      -DOSXCROSS_HOST=${OSXCROSS_HOST} \
      -DOSXCROSS_TARGET_DIR=${OSXCROSS_TARGET_DIR} \
      -DOSXCROSS_SDK=${OSXCROSS_SDK} \
      -DOSXCROSS_TARGET=${OSXCROSS_TARGET} \
      -DCMAKE_BUILD_TYPE=Release \
      -DPYBIND11_NOPYTHON=1 \
      -DPython3_ROOT_DIR=/deps \
      -DPython3_LIBRARY=/deps/lib/libpython3.10.a \
      -DPython3_INCLUDE_DIR=/deps/include/python3.10 \
      -Dpybind11_DIR=/deps/share/cmake/pybind11 \
      -Dspdlog_DIR=/deps/lib/cmake/spdlog \
      -DLLVM_DIR=/deps/LLVM21-NDK29-Darwin/lib/cmake/llvm \
      -DOMVLL_ABI=Android
ninja -C ndk-x86_64

# Xcode 27's clang is arm64-only, so the Apple plugin ships as a single slice.
cp ./xcode-arm64/libOMVLL.dylib omvll-xcode_unsigned.dylib
lipo -create -output omvll-ndk_unsigned.dylib ./ndk-arm64/libOMVLL.dylib ./ndk-x86_64/libOMVLL.dylib

chown -R 1000:1000 /o-mvll/src/build_xcode
chmod -R 777 /o-mvll/src/build_xcode

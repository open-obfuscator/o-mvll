#!/usr/bin/sh

#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

set -ex

mkdir -p /deps && cd /deps

cp /third-party/omvll-deps-xcode-*/LLVM-17.0.6git-arm64-Darwin.tar.gz .
cp /third-party/omvll-deps-xcode-*/LLVM-17.0.6git-x86_64-Darwin.tar.gz .
cp /third-party/omvll-deps-xcode-*/Python-slim.tar.gz .
cp /third-party/omvll-deps-xcode-*/pybind11.tar.gz .
cp /third-party/omvll-deps-xcode-*/spdlog-1.10.0-Darwin.tar.gz .

tar xzvf LLVM-17.0.6git-arm64-Darwin.tar.gz
tar xzvf LLVM-17.0.6git-x86_64-Darwin.tar.gz
tar xzvf Python-slim.tar.gz
tar xzvf pybind11.tar.gz
tar xzvf spdlog-1.10.0-Darwin.tar.gz

cd /o-mvll/src
mkdir -p build_xcode && cd build_xcode

export OSXCROSS_TARGET_DIR=/osxcross
export OSXCROSS_SDK=${OSXCROSS_TARGET_DIR}/SDK/MacOSX15.2.sdk
export OMVLL_PYTHONPATH=/omvll/ci/distribution/Python-3.10.7/Lib
mkdir -p arm64 && cd arm64

export OSXCROSS_HOST="arm64-apple-darwin24.2"
export OSXCROSS_TARGET="arm64-apple-darwin24.2"

cmake -GNinja ../.. \
      -DCMAKE_TOOLCHAIN_FILE=${OSXCROSS_TARGET_DIR}/toolchain.cmake \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="15.2" \
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
      -DLLVM_DIR=/deps/LLVM-17.0.6git-arm64-Darwin/lib/cmake/llvm

ninja

cd ..
mkdir -p x86_64 && cd x86_64

export OSXCROSS_HOST="x86_64-apple-darwin24.2"
export OSXCROSS_TARGET="x86_64-apple-darwin24.2"

cmake -GNinja ../.. \
      -DCMAKE_TOOLCHAIN_FILE=${OSXCROSS_TARGET_DIR}/toolchain.cmake \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="15.2" \
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
      -DLLVM_DIR=/deps/LLVM-17.0.6git-x86_64-Darwin/lib/cmake/llvm

ninja
cd ..

lipo -create -output omvll.dylib ./arm64/libOMVLL.dylib ./x86_64/libOMVLL.dylib
mv /o-mvll/src/build_xcode/omvll.dylib /o-mvll/src/build_xcode/omvll_unsigned.dylib
chown -R 1000:1000 /o-mvll/src/build_xcode
chmod -R 777 /o-mvll/src/build_xcode

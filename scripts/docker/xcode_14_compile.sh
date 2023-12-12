#!/usr/bin/sh
set -ex
mkdir -p /deps && cd /deps

cp /third-party/omvll-deps-xcode-*/LLVM-14.0.0git-arm64-Darwin.tar.gz .
cp /third-party/omvll-deps-xcode-*/LLVM-14.0.0git-x86_64-Darwin.tar.gz .
cp /third-party/omvll-deps-xcode-*/Python-slim.tar.gz .
cp /third-party/omvll-deps-xcode-*/pybind11.tar.gz .
cp /third-party/omvll-deps-xcode-*/spdlog-1.10.0-Darwin.tar.gz .

tar xzvf LLVM-14.0.0git-arm64-Darwin.tar.gz
tar xzvf LLVM-14.0.0git-x86_64-Darwin.tar.gz
tar xzvf Python-slim.tar.gz
tar xzvf pybind11.tar.gz
tar xzvf spdlog-1.10.0-Darwin.tar.gz

cd /o-mvll/src
mkdir -p build_xcode && cd build_xcode

export OSXCROSS_TARGET_DIR=/osxcross
export OSXCROSS_SDK=${OSXCROSS_TARGET_DIR}/SDK/MacOSX13.0.sdk

mkdir -p arm64 && cd arm64

export OSXCROSS_HOST="arm64-apple-darwin22"
export OSXCROSS_TARGET="arm64-apple-darwin22"

cmake -GNinja ../.. \
      -DCMAKE_TOOLCHAIN_FILE=${OSXCROSS_TARGET_DIR}/toolchain.cmake \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="13.0" \
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
      -DLLVM_DIR=/deps/LLVM-14.0.0git-arm64-Darwin/lib/cmake/llvm \

ninja

cd ..
mkdir -p x86_64 && cd x86_64

export OSXCROSS_HOST="x86_64-apple-darwin22"
export OSXCROSS_TARGET="x86_64-apple-darwin22"

cmake -GNinja ../.. \
      -DCMAKE_TOOLCHAIN_FILE=${OSXCROSS_TARGET_DIR}/toolchain.cmake \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="13.0" \
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
      -DLLVM_DIR=/deps/LLVM-14.0.0git-x86_64-Darwin/lib/cmake/llvm \

ninja
cd ..

lipo -create -output omvll.dylib ./arm64/libOMVLL.dylib ./x86_64/libOMVLL.dylib
mv /o-mvll/src/build_xcode/omvll.dylib /o-mvll/src/build_xcode/omvll_unsigned.dylib
chown -R 1000:1000 /o-mvll/src/build_xcode
chmod -R 777 /o-mvll/src/build_xcode

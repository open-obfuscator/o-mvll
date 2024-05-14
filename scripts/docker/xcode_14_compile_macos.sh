#!/usr/bin/env bash
set -ex
mkdir -p deps && cd deps

export DEPS_FOLDER=$(pwd)

cp ../tmp/third-party-xcode14/omvll-deps-xcode-*/LLVM-14.0.0git-arm64-Darwin.tar.gz .
cp ../tmp/third-party-xcode14/omvll-deps-xcode-*/LLVM-14.0.0git-x86_64-Darwin.tar.gz .
cp ../tmp/third-party-xcode14/omvll-deps-xcode-*/Python-slim.tar.gz .
cp ../tmp/third-party-xcode14/omvll-deps-xcode-*/pybind11.tar.gz .
cp ../tmp/third-party-xcode14/omvll-deps-xcode-*/spdlog-1.10.0-Darwin.tar.gz .

tar xzvf LLVM-14.0.0git-arm64-Darwin.tar.gz
tar xzvf LLVM-14.0.0git-x86_64-Darwin.tar.gz
tar xzvf Python-slim.tar.gz
tar xzvf pybind11.tar.gz
tar xzvf spdlog-1.10.0-Darwin.tar.gz

cd ../src
mkdir -p build_xcode && cd build_xcode

# Need to execute ninja check
export OMVLL_PYTHONPATH=${DIST_FOLDER}/Python-3.10.7/Lib

mkdir -p arm64 && cd arm64

cmake -GNinja ../.. \
      -DCMAKE_OSX_ARCHITECTURES="arm64" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="13.0" \
      -DCMAKE_CXX_COMPILER=${DEPS_FOLDER}/LLVM-14.0.0git-arm64-Darwin/bin/clang++ \
      -DCMAKE_C_COMPILER=${DEPS_FOLDER}/LLVM-14.0.0git-arm64-Darwin/bin/clang \
      -DCMAKE_BUILD_TYPE=Release \
      -DPYBIND11_NOPYTHON=1 \
      -DPython3_ROOT_DIR=${DEPS_FOLDER} \
      -DPython3_LIBRARY=${DEPS_FOLDER}/lib/libpython3.10.a \
      -DPython3_INCLUDE_DIR=${DEPS_FOLDER}/include/python3.10 \
      -Dpybind11_DIR=${DEPS_FOLDER}/share/cmake/pybind11 \
      -Dspdlog_DIR=${DEPS_FOLDER}/lib/cmake/spdlog \
      -DLLVM_DIR=${DEPS_FOLDER}/LLVM-14.0.0git-arm64-Darwin/lib/cmake/llvm \
      -DLLVM_TOOLS_DIR=${OMVLL_LLVM_TOOLS_DIR} \
      -DLLVM_EXTERNAL_LIT=${OMVLL_LLVM_TOOLS_DIR}/bin/llvm-lit

ninja check

cd ..
mkdir -p x86_64 && cd x86_64

cmake -GNinja ../.. \
      -DCMAKE_OSX_ARCHITECTURES="x86_64" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="13.0" \
      -DCMAKE_CXX_COMPILER=${DEPS_FOLDER}/LLVM-14.0.0git-x86_64-Darwin/bin/clang++ \
      -DCMAKE_C_COMPILER=${DEPS_FOLDER}/LLVM-14.0.0git-x86_64-Darwin/bin/clang \
      -DCMAKE_BUILD_TYPE=Release \
      -DPYBIND11_NOPYTHON=1 \
      -DPython3_ROOT_DIR=${DEPS_FOLDER} \
      -DPython3_LIBRARY=${DEPS_FOLDER}/lib/libpython3.10.a \
      -DPython3_INCLUDE_DIR=${DEPS_FOLDER}/include/python3.10 \
      -Dpybind11_DIR=${DEPS_FOLDER}/share/cmake/pybind11 \
      -Dspdlog_DIR=${DEPS_FOLDER}/lib/cmake/spdlog \
      -DLLVM_DIR=${DEPS_FOLDER}/LLVM-14.0.0git-x86_64-Darwin/lib/cmake/llvm \
      -DLLVM_TOOLS_DIR=${OMVLL_LLVM_TOOLS_DIR} \
      -DLLVM_EXTERNAL_LIT=${OMVLL_LLVM_TOOLS_DIR}/bin/llvm-lit

ninja
cd ..

lipo -create -output omvll.dylib ./arm64/libOMVLL.dylib ./x86_64/libOMVLL.dylib
mv ./omvll.dylib ./omvll_unsigned.dylib

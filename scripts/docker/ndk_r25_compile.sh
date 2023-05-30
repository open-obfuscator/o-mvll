#!/usr/bin/sh
set -ex

mkdir -p /deps && cd /deps

cp /third-party/LLVM-14.0.6git-Linux-slim.tar.gz .
cp /third-party/Python-slim.tar.gz .
cp /third-party/pybind11.tar.gz .
cp /third-party/spdlog-1.10.0-Linux.tar.gz .

tar xzvf LLVM-14.0.6git-Linux-slim.tar.gz
tar xzvf Python-slim.tar.gz
tar xzvf pybind11.tar.gz
tar xzvf spdlog-1.10.0-Linux.tar.gz

cd /o-mvll/src
mkdir -p build_ndk_r25 && cd build_ndk_r25
cmake -GNinja .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_COMPILER=clang++-14 \
      -DCMAKE_C_COMPILER=clang-14 \
      -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
      -DCMAKE_SKIP_RPATH=ON \
      -DCMAKE_SKIP_BUILD_RPATH=ON \
      -DPython3_ROOT_DIR=/deps \
      -DPython3_LIBRARY=/deps/lib/libpython3.10.a \
      -DPython3_INCLUDE_DIR=/deps/include/python3.10 \
      -Dpybind11_DIR=/deps/share/cmake/pybind11 \
      -Dspdlog_DIR=/deps/lib/cmake/spdlog \
      -DLLVM_DIR=/deps/LLVM-14.0.6git-Linux/lib64/cmake/llvm \
      -DClang_DIR=/deps/LLVM-14.0.6git-Linux/lib64/cmake/clang \
      -DLLVM_TOOLS_DIR=/test-deps \
      -DLLVM_EXTERNAL_LIT=/usr/local/bin/lit

ninja

export OMVLL_PYTHONPATH=/Python-3.10.7/Lib
ninja check

mkdir -p /o-mvll/dist
python3 /o-mvll/scripts/package.py -t ndk_r25 /o-mvll/src/build_ndk_r25/libOMVLL.so /o-mvll/dist/omvll_ndk_r25.tar.gz

chown -R 1000:1000 /o-mvll/src/build_ndk_r25
chown -R 1000:1000 /o-mvll/dist

#!/usr/bin/sh
set -ex

mkdir -p /deps && cd /deps

cp /third-party/LLVM-14.0.6git-Linux-slim-doc.tar.gz .
cp /third-party/pybind11.tar.gz .
cp /third-party/spdlog-1.10.0-Linux.tar.gz .

tar xzvf LLVM-14.0.6git-Linux-slim-doc.tar.gz
tar xzvf pybind11.tar.gz
tar xzvf spdlog-1.10.0-Linux.tar.gz

mkdir -p /standalone
cmake -GNinja -S /o-mvll/src -B /standalone                  \
      -DOMVLL_PY_STANDALONE=1                                \
      -DCMAKE_BUILD_TYPE=Release                             \
      -DCMAKE_CXX_COMPILER=clang++-11                        \
      -DCMAKE_C_COMPILER=clang-11                            \
      -DCMAKE_CXX_FLAGS="-stdlib=libc++ -Wl,-lunwind -Wno-unused-command-line-argument" \
      -DCMAKE_SKIP_RPATH=ON                                  \
      -DCMAKE_SKIP_BUILD_RPATH=ON                            \
      -DPYBIND11_NOPYTHON=1                                  \
      -DPYTHON_EXECUTABLE=/usr/local/bin/python3             \
      -DPython3_EXECUTABLE=/usr/local/bin/python3            \
      -DPython3_ROOT_DIR=/usr                                \
      -DPython3_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.10.so.1.0  \
      -DPython3_INCLUDE_DIR=/usr/local/include/python3.10    \
      -Dpybind11_DIR=/deps/share/cmake/pybind11              \
      -Dspdlog_DIR=/deps/lib/cmake/spdlog                    \
      -DLLVM_DIR=/deps/LLVM-14.0.6git-Linux/lib64/cmake/llvm \
      -DClang_DIR=/deps/LLVM-14.0.6git-Linux/lib64/cmake/clang
ninja -C /standalone

OMVLL_STANDALONE_DIR=/standalone \
sphinx-build -a -E -j4 -w /tmp/sphinx-warn.log /o-mvll/doc /doc

chown -R 1000:1000 /doc

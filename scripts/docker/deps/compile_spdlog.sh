#!/usr/bin/sh
# This script is used to compile cpython
set -ex

cd /spdlog

curl -LO https://github.com/gabime/spdlog/archive/refs/tags/v1.10.0.tar.gz
tar xzvf v1.10.0.tar.gz
cd spdlog-1.10.0

export CXXFLAGS="-stdlib=libc++ -fPIC -fno-rtti -fno-exceptions -fvisibility-inlines-hidden"
cmake -GNinja -S . -B /tmp/spdlog_build \
      -DCMAKE_CXX_COMPILER=clang++-11 \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
      -DSPDLOG_NO_THREAD_ID=on \
      -DSPDLOG_NO_TLS=on \
      -DSPDLOG_NO_EXCEPTIONS=on \
      -DSPDLOG_BUILD_EXAMPLE=off \
      -DCMAKE_INSTALL_PREFIX=./install
ninja -vvv -C /tmp/spdlog_build package
cp /tmp/spdlog_build/spdlog-1.10.0-Linux.tar.gz /spdlog/

rm -rf /spdlog/v1.10.0.tar.gz && rm -rf spdlog-1.10.0
chown 1000:1000 /spdlog/spdlog-1.10.0-Linux.tar.gz

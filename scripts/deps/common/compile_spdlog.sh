#!/usr/bin/env bash
# This script is used to compile cpython
set -e

curl -LO https://github.com/gabime/spdlog/archive/refs/tags/v1.10.0.tar.gz
tar xzvf v1.10.0.tar.gz
cd spdlog-1.10.0

export CXXFLAGS="-fPIC -fno-rtti -fno-exceptions -fvisibility-inlines-hidden"
cmake -GNinja -S . -B ./tmp/spdlog_build \
      -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"      \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="14.0"          \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
      -DSPDLOG_NO_THREAD_ID=on \
      -DSPDLOG_NO_TLS=on \
      -DSPDLOG_NO_EXCEPTIONS=on \
      -DSPDLOG_BUILD_EXAMPLE=off \
      -DCMAKE_INSTALL_PREFIX=./install
ninja -vvv -C ./tmp/spdlog_build package
cd ..
cp ./spdlog-1.10.0/tmp/spdlog_build/spdlog-1.10.0-*.tar.gz ./omvll-deps

rm -rf ./v1.10.0.tar.gz && rm -rf ./spdlog-1.10.0

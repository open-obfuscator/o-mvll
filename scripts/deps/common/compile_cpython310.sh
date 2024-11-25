#!/usr/bin/env bash

#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

# This script is used to compile cpython

set -e

curl -LO https://www.python.org/ftp/python/3.10.7/Python-3.10.7.tgz
tar xzvf Python-3.10.7.tgz
cd Python-3.10.7

./configure \
  --disable-ipv6                    \
  --prefix=$(pwd)/cpython-install         \
  --enable-universalsdk             \
  --with-universal-archs=universal2 \
  --disable-test-modules            \
  ac_default_prefix=./install
make -j$(nproc) install

cd ./cpython-install
tar czvf Python-slim.tar.gz include/python3.10/ lib/libpython3.10.a lib/pkgconfig/ share/
cd ../../

cp Python-3.10.7/cpython-install/Python-slim.tar.gz ./omvll-deps/
rm -rf Python-3.10.7 && rm -rf Python-3.10.7.tgz

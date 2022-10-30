#!/usr/bin/sh
# This script is used to compile cpython
set -ex

cd /cpython

curl -LO https://www.python.org/ftp/python/3.10.7/Python-3.10.7.tgz
tar xzvf Python-3.10.7.tgz
cd Python-3.10.7

export CC=clang
export CXX=clang++
export CFLAGS="-fPIC -m64"

./configure \
  --disable-ipv6           \
  --host=x86_64-linux-gnu \
  --target=x86_64-linux-gnu \
  --build=x86_64-linux-gnu \
  --prefix=/cpython-install \
  --disable-test-modules
make -j$(nproc) install

pushd /cpython-install
tar czvf Python.tar.gz *
popd

cd /cpython
rm -rf Python-3.10.7 && rm -rf Python-3.10.7.tgz
cp /cpython-install/Python.tar.gz /cpython/
chown 1000:1000 /cpython/Python.tar.gz


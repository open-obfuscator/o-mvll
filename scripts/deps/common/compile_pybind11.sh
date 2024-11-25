#!/usr/bin/env bash

#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

# This script is used to compile pybind11

set -e

curl -LO https://github.com/pybind/pybind11/archive/refs/tags/v2.10.3.tar.gz
tar xzvf v2.10.3.tar.gz
cd pybind11-2.10.3
python3 setup.py build
tar -C build/lib/pybind11/ -f ../omvll-deps/pybind11.tar.gz -cvz include/pybind11/ share/cmake/

cd ..
rm -rf v2.10.3.tar.gz && rm -rf pybind11-2.10.3

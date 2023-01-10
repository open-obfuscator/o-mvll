#!/usr/bin/sh
# This script is used to compile pybind11
set -ex

cd /pybind11

curl -LO https://github.com/pybind/pybind11/archive/refs/tags/v2.10.3.tar.gz
tar xzvf v2.10.3.tar.gz
cd pybind11-2.10.3
python3 setup.py build
tar -C build/lib/pybind11/ -f ../pybind11.tar.gz -cvz include/pybind11/ share/cmake/

rm -rf /pybind11/v2.10.3.tar.gz && rm -rf /pybind11/pybind11-2.10.3
chown 1000:1000 /pybind11/pybind11.tar.gz

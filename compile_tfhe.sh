#!/bin/bash

cd tfhe
mkdir -p build
cd build

export CC=/tools/Xilinx/Vitis/2020.1/bin/clang_wrapper
export CXX=/tools/Xilinx/Vitis/2020.1/bin/clang_wrapper

cmake ../src -DCMAKE_INSTALL_PREFIX=./../../ -DCMAKE_BUILD_TYPE=optim -DENABLE_NAYUKI_PORTABLE=on

make
make install

cd ..
rm -rf ./build/
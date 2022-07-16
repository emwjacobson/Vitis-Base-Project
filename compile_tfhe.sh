#!/bin/bash

cd tfhe
mkdir -p build
cd build

export CC=/tools/Xilinx/Vitis/2020.1/bin/clang_wrapper
export CXX=/tools/Xilinx/Vitis/2020.1/bin/clang_wrapper

cmake ../src -DCMAKE_INSTALL_PREFIX=./../../ -DENABLE_NAYUKI_PORTABLE=on -DENABLE_FFTW=off -DENABLE_NAYUKI_AVX=off -DENABLE_SPQLIOS_AVX=off -DENABLE_SPQLIOS_FMA=off

make -j
make install

cd ..
rm -rf ./build/
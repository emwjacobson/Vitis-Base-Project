TARGET = hw
PLATFORM = xilinx_u280_xdma_201920_3
VPP = v++
GPP = g++
KERNEL_NAME = test_kernel
CONFIG_NAME = config.cfg
SOURCE_FILES = ./src/host.cpp
TFHE_SOURCES := $(shell find ./tfhe/src/libtfhe -name *.cpp)

all: app ${KERNEL_NAME}.xclbin

sw_emu: TARGET = sw_emu
sw_emu: clean all
	emconfigutil --platform ${PLATFORM} --nd 1
#	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib/ XCL_EMULATION_MODE=sw_emu ./app

hw_emu: TARGET = hw_emu
hw_emu: clean all
	emconfigutil --platform ${PLATFORM} --nd 1
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib/ XCL_EMULATION_MODE=hw_emu ./app

app: ./src/host.cpp
	${GPP} -Wall -g -std=c++11 ./src/host.cpp -o app \
		-I${XILINX_XRT}/include/ \
		-L${XILINX_XRT}/lib/ \
		-I${CURDIR}/include/ \
		-L${CURDIR}/lib/ \
		-ltfhe-nayuki-portable \
		-lOpenCL -lrt -lstdc++ \
		-DKERNEL_NAME=\"${KERNEL_NAME}\"

${KERNEL_NAME}.xo: ./src/${KERNEL_NAME}.cpp
	${VPP} -c --config ${CONFIG_NAME} -t ${TARGET} -f ${PLATFORM} -k ${KERNEL_NAME} \
		-I./src \
		-I${CURDIR}/include/ \
		-I${CURDIR}/include/tfhe \
		-I${CURDIR}/tfhe/src/libtfhe/fft_processors/nayuki/ \
		./src/${KERNEL_NAME}.cpp ${TFHE_SOURCES} -o ${KERNEL_NAME}.xo

${KERNEL_NAME}.xclbin: ./${KERNEL_NAME}.xo
	${VPP} -l --profile_kernel data:all:all:all --config ${CONFIG_NAME} -t ${TARGET} -f ${PLATFORM} ./${KERNEL_NAME}.xo -o ${KERNEL_NAME}.xclbin

clean:
	rm -rf ${KERNEL_NAME}* app *json *csv *log *summary _x .run .Xil .ipcache *.jou cloud.key secret.key answer.data cloud.data
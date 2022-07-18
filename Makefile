PLATFORM = xilinx_u280_xdma_201920_3
CONFIG_NAME := config.cfg
TARGET := sw_emu
PROJECT_NAME := maths

KERNEL_XO := vadd.xo
BINS := host.cpp cloud.cpp verify.cpp
SRC := utils.cpp cloudcompute.cpp

VPP := v++
VPP_XO_FLAGS := -c --platform $(PLATFORM)
VPP_XCLBIN_FLAGS := -l --profile_kernel data:all:all:all -O1 --platform $(PLATFORM) -t $(TARGET) --config $(CONFIG_NAME) $(KERNEL_XO) -o $(PROJECT_NAME).xclbin

CXX_FLAGS := -Wall -c -g -std=c++11 -O1
CXX_INCLUDES := -I$(XILINX_XRT)/include/ -I$(CURDIR)/include/
CXX_LIB := -L$(XILINX_XRT)/lib/ -L$(CURDIR)/lib/ -ltfhe-nayuki-portable -lOpenCL -lpthread -lrt -lstdc++

# tfhe-nayuki-avx
# tfhe-nayuki-portable
# tfhe-spqlios-avx
# tfhe-spqlios-fma

all: xclbin host

sw_emu:
	emconfigutil --platform $(PLATFORM) --nd 1
	XCL_EMULATION_MODE=sw_emu ./$(PROJECT_NAME)

hw_emu:
	emconfigutil --platform $(PLATFORM) --nd 1
	XCL_EMULATION_MODE=hw_emu ./$(PROJECT_NAME)

host: $(BINS) $(SRC)
	$(CXX) host.o $(addsuffix .o, $(basename $(SRC))) $(CXX_LIB) -O1 -std=c++11 -o host
	$(CXX) cloud.o $(addsuffix .o, $(basename $(SRC))) $(CXX_LIB) -O1 -std=c++11 -o cloud
	$(CXX) verify.o $(addsuffix .o, $(basename $(SRC))) $(CXX_LIB) -O1 -std=c++11 -o verify

%.cpp: src/%.cpp
	$(CXX) $(CXX_FLAGS) $(CXX_INCLUDES) $< -o $(basename $@).o

xclbin: $(KERNEL_XO)
	$(VPP) $(VPP_XCLBIN_FLAGS)

%.xo: kernels/%.cpp
	$(VPP) $(VPP_XO_FLAGS) -k $(basename $(notdir $<)) $< -o $@

clean:
	rm -rf *json *csv *log *summary _x .run .Xil .ipcache *.jou $(KERNEL_XO) *.xclbin* $(PROJECT_NAME) *.o *.xo *.key *.data callgrind* vitis_analyzer* cloud host verify

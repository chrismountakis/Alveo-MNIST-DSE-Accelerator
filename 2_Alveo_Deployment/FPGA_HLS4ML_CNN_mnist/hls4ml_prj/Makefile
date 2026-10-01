############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86> EDGE_COMMON_SW=<rootfs and kernel image path> CONFIG_FILE=<path to cfg file>"
	$(ECHO) "  make clean "
	$(ECHO) "  make cleanall"
	$(ECHO) "  make test DEVICE=<FPGA platform>"
	$(ECHO) "  make run TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86> EDGE_COMMON_SW=<rootfs and kernel image path>"
	$(ECHO) "  make build TARGET=<sw_emu/hw_emu/hw> DEVICE=<FPGA platform> HOST_ARCH=<aarch32/aarch64/x86> EDGE_COMMON_SW=<rootfs and kernel image path> CONFIG_FILE=<path to cfg file>"
	$(ECHO) "  make host HOST_ARCH=<aarch32/aarch64/x86> EDGE_COMMON_SW=<rootfs and kernel image path>"

############################## Setting up Project Variables ##############################
MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
COMMON_REPO ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%lsal/*}')
PWD = $(shell readlink -f .)
XF_PROJ_ROOT = $(shell readlink -f $(COMMON_REPO))

TARGET := hw
HOST_ARCH := x86
SYSROOT := 
CONFIG_FILE ?= firmware/myproject.cfg

include ./utils.mk

XSA := 
ifneq ($(DEVICE), )
XSA := $(call device2xsa, $(DEVICE))
endif
TEMP_DIR := ./_x.$(TARGET).$(XSA)
BUILD_DIR := ./build_dir.$(TARGET).$(XSA)

RUN_APP_SCRIPT = ./run_app.sh
PACKAGE_OUT = ./package.$(TARGET)

LAUNCH_EMULATOR = $(PACKAGE_OUT)/launch_$(TARGET).sh
RESULT_STRING = TEST PASSED

VPP := v++
CMD_ARGS = $(BUILD_DIR)/myproject.xclbin

include ../common/includes/opencl/opencl.mk
CXXFLAGS += $(opencl_CXXFLAGS) -Wall -O3 -g -std=c++11
LDFLAGS += $(opencl_LDFLAGS)

ifeq ($(findstring nodma, $(DEVICE)), nodma)
$(error [ERROR]: This example is not supported for $(DEVICE).)
endif

CXXFLAGS += -I$(XF_PROJ_ROOT)/common/includes/xcl2
HOST_SRCS += $(XF_PROJ_ROOT)/common/includes/xcl2/xcl2.cpp ./firmware/host.cpp
CXXFLAGS += -I/opt/xilinx/Vitis_HLS/2023.1/include
CXXFLAGS += -I/opt/xilinx/Vitis_HLS/2023.1/vcxx/data/include
CXXFLAGS += -fmessage-length=0
LDFLAGS += -lrt -lstdc++ -lpthread -lOpenCL -lxrt_coreutil -lxrt_core

ifneq ($(HOST_ARCH), x86)
	LDFLAGS += --sysroot=$(SYSROOT)
endif

VPP_FLAGS += -O3 -t $(TARGET) --platform $(DEVICE) --save-temps 
ifneq ($(TARGET), hw)
	VPP_FLAGS += -g
endif
ifeq ($(TARGET), hw)
	VPP_PROFILE_FLAGS += -g --profile.data all:all:all --profile.stall all:all:all --profile.exec all:all:all
endif

EXECUTABLE = ./myproject_host
EMCONFIG_DIR = $(TEMP_DIR)

BINARY_CONTAINERS += $(BUILD_DIR)/myproject.xclbin
BINARY_CONTAINER_myproject_OBJS += $(TEMP_DIR)/myproject.xo
BINARY_CONTAINER_mm2s_OBJS += $(TEMP_DIR)/mm2s.xo
BINARY_CONTAINER_s2mm_OBJS += $(TEMP_DIR)/s2mm.xo

.PHONY: all clean cleanall docs emconfig
all: check-devices $(EXECUTABLE) $(BINARY_CONTAINERS) emconfig

.PHONY: host
host: $(EXECUTABLE)

.PHONY: build
build: check-vitis $(BINARY_CONTAINERS)

.PHONY: xclbin
xclbin: build

######################### Kernel Compilation #########################
$(TEMP_DIR)/myproject.xo: firmware/myproject.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k myproject --temp_dir $(TEMP_DIR) -I'firmware' -o'$@' '$<'

$(TEMP_DIR)/mm2s.xo: firmware/mm2s.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k mm2s --temp_dir $(TEMP_DIR) -I'firmware' -o'$@' '$<'

$(TEMP_DIR)/s2mm.xo: firmware/s2mm.cpp
	mkdir -p $(TEMP_DIR)
	$(VPP) $(VPP_FLAGS) -c -k s2mm --temp_dir $(TEMP_DIR) -I'firmware' -o'$@' '$<'

######################### XCLBIN Linking #########################
$(BUILD_DIR)/myproject.xclbin: $(BINARY_CONTAINER_myproject_OBJS) $(BINARY_CONTAINER_mm2s_OBJS) $(BINARY_CONTAINER_s2mm_OBJS)
	mkdir -p $(BUILD_DIR)
	$(VPP) $(VPP_FLAGS) -l $(VPP_LDFLAGS) $(VPP_PROFILE_FLAGS) --temp_dir $(BUILD_DIR) --config $(CONFIG_FILE) -o $(BUILD_DIR)/myproject.xclbin $^

######################### Host Build #########################
$(EXECUTABLE): $(HOST_SRCS) | check-xrt
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

######################### Emconfig #########################
emconfig:$(EMCONFIG_DIR)/emconfig.json
$(EMCONFIG_DIR)/emconfig.json:
	emconfigutil --platform $(DEVICE) --od $(EMCONFIG_DIR)

######################### Run #########################
run: all
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
ifeq ($(HOST_ARCH), x86)
	$(CP) $(EMCONFIG_DIR)/emconfig.json .
	XCL_EMULATION_MODE=$(TARGET) $(EXECUTABLE) $(CMD_ARGS)
else
	$(LAUNCH_EMULATOR_CMD)
endif
else
ifeq ($(HOST_ARCH), x86)
	$(EXECUTABLE) $(CMD_ARGS)
endif
endif

.PHONY: test
test: $(EXECUTABLE)
ifeq ($(TARGET),$(filter $(TARGET),sw_emu hw_emu))
ifeq ($(HOST_ARCH), x86)
	XCL_EMULATION_MODE=$(TARGET) $(EXECUTABLE) $(CMD_ARGS)
else
	$(LAUNCH_EMULATOR_CMD)
endif
else
ifeq ($(HOST_ARCH), x86)
	$(EXECUTABLE) $(CMD_ARGS)
endif
endif

######################### Clean #########################
clean:
	-$(RMDIR) $(EXECUTABLE) $(XCLBIN)/{*sw_emu*,*hw_emu*} 
	-$(RMDIR) profile_* TempConfig system_estimate.xtxt *.rpt *.csv 
	-$(RMDIR) firmware/*.ll *v++* .Xil emconfig.json dltmp* xmltmp* *.log *.jou *.wcfg *.wdb

cleanall: clean
	-$(RMDIR) build_dir*
	-$(RMDIR) package.*
	-$(RMDIR) _x* *xclbin.run_summary *xclbin.package_summary qemu-memory-_* emulation _vimage pl* start_simulation.sh *.xclbin

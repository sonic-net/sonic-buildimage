################################################################################
# Copyright (C) 2024  Clounix, Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# version 2 along with this program.
################################################################################
################################################################################
KERNEL_MODULE := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))

################################################################################
MAKE                := $(shell which make)
RM                  := rm -rf
MKDIR               := mkdir -p
CP                  := cp
MV                  := mv
TEST_PATH           := test -d

KVERSION            ?= $(shell uname -r)
OS_PATH             := /lib/modules/$(KVERSION)/build
KNET_INC_PATH		:= $(KERNEL_MODULE)/linux/include/
MT_INC_PATH			:= $(KERNEL_MODULE)/linux/include/nb/
BUILD_OUTPUT_DIR    := $(KERNEL_MODULE)/build
MODULE_OUTPUT_DIR   := $(BUILD_OUTPUT_DIR)/module

all: compile install
################################################################################
EXTRA_CFLAGS += -I$(KNET_INC_PATH)
EXTRA_CFLAGS += -I$(MT_INC_PATH)
EXTRA_CFLAGS += -DCLX_EN_NETIF
EXTRA_CFLAGS += -DCLX_EN_DEBUG
EXTRA_CFLAGS += -DCLX_LINUX_USER_MODE
EXTRA_CFLAGS += -DCLX_EN_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCLX_EN_COMPILER_SUPPORT_FUNCTION
EXTRA_CFLAGS += -DCLX_EN_COMPILER_SUPPORT_LONG_LONG

ifeq ($(shell uname -m),x86_64)
EXTRA_CFLAGS += -DCLX_EN_HOST_64_BIT_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCLX_EN_64BIT_ADDR
else
ifeq ($(shell uname -m),aarch64)
EXTRA_CFLAGS += -DCLX_EN_HOST_64_BIT_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCLX_EN_64BIT_ADDR
else
EXTRA_CFLAGS += -DCLX_EN_HOST_32_BIT_LITTLE_ENDIAN
endif
endif

COMPILE_TIME := $(shell date -u +'%Y-%m-%d_%H:%M:%S-%Z')
EXTRA_CFLAGS += -DKNET_DEV_MAKE_COMPILE_TIME=\"$(COMPILE_TIME)\"
DIR_TO_CHECK_FOR_SDK := ../../../../.git
DIR_TO_CHECK_FOR_KNET := .git
ifneq ($(or $(wildcard $(DIR_TO_CHECK_FOR_SDK)), $(wildcard $(DIR_TO_CHECK_FOR_KNET))),)
GIT_HASH := $(shell git rev-parse --short HEAD 2> /dev/null)
EXTRA_CFLAGS += -DKNET_DEV_MAKE_GIT_HASH=\"$(GIT_HASH)\"
else
EXTRA_CFLAGS += -DKNET_DEV_RELEASED
endif

################################################################################
DEV_MODULE_NAME            := clx_dev

################################################################################
DEV_OBJS_TOTAL             := linux/src/knet_dev.o
DEV_OBJS_TOTAL             += linux/src/knet_pci.o
DEV_OBJS_TOTAL             += linux/src/knet_intr.o
DEV_OBJS_TOTAL             += linux/src/knet_dma.o
DEV_OBJS_TOTAL             += linux/src/knet_buffer.o
DEV_OBJS_TOTAL             += linux/src/knet_drv.o
DEV_OBJS_TOTAL             += linux/src/knet_netif.o
DEV_OBJS_TOTAL             += linux/src/knet_netlink.o

CHIP_OBJS_TOTAL            := linux/src/nb/nb_chip.o
CHIP_OBJS_TOTAL            += linux/src/nb/nb_drv.o


obj-m                      := $(DEV_MODULE_NAME).o
$(DEV_MODULE_NAME)-objs    := $(DEV_OBJS_TOTAL)
$(DEV_MODULE_NAME)-objs    += $(CHIP_OBJS_TOTAL)

KBUILD_EXTRA_SYMBOLS       := $(BUILD_OUTPUT_DIR)/Module.symvers
################################################################################
folder:
	$(TEST_PATH) $(BUILD_OUTPUT_DIR) || $(MKDIR) $(BUILD_OUTPUT_DIR)
	$(TEST_PATH) $(BUILD_OUTPUT_DIR)/src || $(MKDIR) $(BUILD_OUTPUT_DIR)/src

compile:: folder
	touch $(BUILD_OUTPUT_DIR)/Makefile
	touch $(KBUILD_EXTRA_SYMBOLS)
	$(MAKE) -C $(OS_PATH) M=$(BUILD_OUTPUT_DIR) src=$(shell pwd) modules EXTRA_CFLAGS="$(EXTRA_CFLAGS)" KBUILD_EXTRA_SYMBOLS=$(KBUILD_EXTRA_SYMBOLS)

install::
	$(TEST_PATH) $(MODULE_OUTPUT_DIR) || $(MKDIR) $(MODULE_OUTPUT_DIR)
	$(MV) $(BUILD_OUTPUT_DIR)/$(DEV_MODULE_NAME).ko $(MODULE_OUTPUT_DIR)/$(DEV_MODULE_NAME).ko

clean::
	$(RM) $(BUILD_OUTPUT_DIR)

.PHONY: all compile install clean
.NOTPARALLEL: all compile install clean


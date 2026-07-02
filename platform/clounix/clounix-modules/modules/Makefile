################################################################################
# Copyright (C) 2021  Clounix, Inc.
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

#KVERSION            ?= $(shell uname -r)
KVERSION            ?= 4.19.0-12-2-amd64
OS_PATH             := /lib/modules/$(KVERSION)/build
SRC_PATH            := $(KERNEL_MODULE)/src
INC_PATH            := $(SRC_PATH)/inc
BUILD_OUTPUT_DIR    := $(KERNEL_MODULE)/build
MODULE_OUTPUT_DIR   := $(BUILD_OUTPUT_DIR)/module

all: compile install
################################################################################
EXTRA_CFLAGS += -I$(INC_PATH)
EXTRA_CFLAGS += -I$(SRC_PATH)/clx_netif/inc/
EXTRA_CFLAGS += -DCLX_EN_NETIF
EXTRA_CFLAGS += -DCLX_EN_DAWN
EXTRA_CFLAGS += -DCLX_EN_LIGHTNING
EXTRA_CFLAGS += -DCLX_EN_DEBUG
EXTRA_CFLAGS += -DCLX_LINUX_USER_MODE
EXTRA_CFLAGS += -DCLX_EN_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCLX_EN_COMPILER_SUPPORT_FUNCTION
EXTRA_CFLAGS += -DCLX_EN_COMPILER_SUPPORT_LONG_LONG

ifeq ($(shell uname -m),x86_64)
EXTRA_CFLAGS += -DCLX_EN_HOST_64_BIT_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCLX_EN_64BIT_ADDR
else
EXTRA_CFLAGS += -DCLX_EN_HOST_32_BIT_LITTLE_ENDIAN
endif

################################################################################
DEV_MODULE_NAME            := clx_dev
NETIF_MODULE_NAME          := clx_netif

################################################################################
DEV_OBJS_TOTAL             := ./src/clx_dev/osal_mdc.o
DEV_OBJS_TOTAL             += ./src/clx_dev/osal_isymbol.o

NETIF_OBJS_TOTAL           := ./src/clx_netif/netif_knl.o
NETIF_OBJS_TOTAL           += ./src/clx_netif/netif_osal.o
NETIF_OBJS_TOTAL           += ./src/clx_netif/netif_perf.o
NETIF_OBJS_TOTAL           += ./src/clx_netif/netif_nl.o
NETIF_OBJS_TOTAL           += ./src/clx_netif/hal_dawn_pkt_knl.o
NETIF_OBJS_TOTAL           += ./src/clx_netif/hal_lightning_pkt_knl.o

obj-m                      := $(DEV_MODULE_NAME).o $(NETIF_MODULE_NAME).o
$(DEV_MODULE_NAME)-objs    := $(DEV_OBJS_TOTAL)
$(NETIF_MODULE_NAME)-objs  := $(NETIF_OBJS_TOTAL)

KBUILD_EXTRA_SYMBOLS       := $(BUILD_OUTPUT_DIR)/Module.symvers
################################################################################
folder:
	$(TEST_PATH) $(BUILD_OUTPUT_DIR) || $(MKDIR) $(BUILD_OUTPUT_DIR)
	$(TEST_PATH) $(BUILD_OUTPUT_DIR)/src || $(MKDIR) $(BUILD_OUTPUT_DIR)/src

compile:: folder
	touch $(BUILD_OUTPUT_DIR)/Makefile
	$(MAKE) -C $(OS_PATH) M=$(BUILD_OUTPUT_DIR) src=$(shell pwd) modules EXTRA_CFLAGS="$(EXTRA_CFLAGS)" KBUILD_EXTRA_SYMBOLS=$(KBUILD_EXTRA_SYMBOLS)

install::
	$(TEST_PATH) $(MODULE_OUTPUT_DIR) || $(MKDIR) $(MODULE_OUTPUT_DIR)
	$(MV) $(BUILD_OUTPUT_DIR)/$(DEV_MODULE_NAME).ko $(MODULE_OUTPUT_DIR)/$(DEV_MODULE_NAME).ko
	$(MV) $(BUILD_OUTPUT_DIR)/$(NETIF_MODULE_NAME).ko $(MODULE_OUTPUT_DIR)/$(NETIF_MODULE_NAME).ko

clean::
	$(RM) $(BUILD_OUTPUT_DIR)

.PHONY: all compile install clean
.NOTPARALLEL: all compile install clean


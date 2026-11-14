################################################################################
# Copyright (C) 2026  Grandtrans, Inc.
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
GT_MODULES_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
SRC_PATH        := $(GT_MODULES_DIR)/src
INC_PATH        := $(GT_MODULES_DIR)/inc
################################################################################
include $(GT_MODULES_DIR)/config.mk
################################################################################
MODULE_OUTPUT_DIR := $(BUILD_OUTPUT_DIR)/module
################################################################################
all: compile install
################################################################################
EXTRA_CFLAGS += -I$(INC_PATH)
EXTRA_CFLAGS += -DGT_EN_NETIF
EXTRA_CFLAGS += -DGT_EN_DEBUG
EXTRA_CFLAGS += -DGT_LINUX_USER_MODE
EXTRA_CFLAGS += -DGT_EN_LITTLE_ENDIAN
EXTRA_CFLAGS += -DGT_EN_COMPILER_SUPPORT_FUNCTION
EXTRA_CFLAGS += -DGT_EN_COMPILER_SUPPORT_LONG_LONG

ifeq ($(shell uname -m),x86_64)
EXTRA_CFLAGS += -DGT_EN_HOST_64_BIT_LITTLE_ENDIAN
EXTRA_CFLAGS += -DGT_EN_64BIT_ADDR
else
ifeq ($(shell uname -m),aarch64)
EXTRA_CFLAGS += -DGT_EN_HOST_64_BIT_LITTLE_ENDIAN
EXTRA_CFLAGS += -DGT_EN_64BIT_ADDR
else
EXTRA_CFLAGS += -DGT_EN_HOST_32_BIT_LITTLE_ENDIAN
endif
endif
################################################################################
include $(SRC_PATH)/make.mk
################################################################################
compile::

install::
	$(TEST_PATH) $(MODULE_OUTPUT_DIR) || $(MKDIR) $(MODULE_OUTPUT_DIR)
	$(MV) $(BUILD_OUTPUT_DIR)/$(NETIF_MODULE_NAME).ko $(MODULE_OUTPUT_DIR)/$(NETIF_MODULE_NAME).ko
	$(MV) $(BUILD_OUTPUT_DIR)/$(GT_DEV_MODULE_NAME).ko $(MODULE_OUTPUT_DIR)/$(GT_DEV_MODULE_NAME).ko

clean::
	$(RM) $(BUILD_OUTPUT_DIR)

.PHONY: all compile install clean
.NOTPARALLEL: all compile install clean

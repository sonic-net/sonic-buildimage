################################################################################
# Copyright (C) 2019  Nephos, Inc.
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
NPS_MODULES_DIR := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
SRC_PATH        := $(NPS_MODULES_DIR)/src
INC_PATH        := $(SRC_PATH)/inc
################################################################################
include $(NPS_MODULES_DIR)/config.mk
################################################################################
MODULE_OUTPUT_DIR := $(BUILD_OUTPUT_DIR)/module
################################################################################
all: compile install
################################################################################
EXTRA_CFLAGS += -I$(INC_PATH)
EXTRA_CFLAGS += -DNPS_EN_NETIF
EXTRA_CFLAGS += -DNPS_EN_TAURUS
EXTRA_CFLAGS += -DNPS_LINUX_USER_MODE
EXTRA_CFLAGS += -DNPS_EN_LITTLE_ENDIAN
ifeq ($(shell uname -m),x86_64)
EXTRA_CFLAGS += -DNPS_EN_HOST_64_BIT_LITTLE_ENDIAN
else
EXTRA_CFLAGS += -DNPS_EN_HOST_32_BIT_LITTLE_ENDIAN
endif
################################################################################
include $(SRC_PATH)/make.mk
################################################################################
compile::

install::
	$(TEST_PATH) $(MODULE_OUTPUT_DIR) || $(MKDIR) $(MODULE_OUTPUT_DIR)
	$(MV) $(BUILD_OUTPUT_DIR)/$(DEV_MODULE_NAME).ko $(MODULE_OUTPUT_DIR)/$(DEV_MODULE_NAME).ko
	$(MV) $(BUILD_OUTPUT_DIR)/$(NETIF_MODULE_NAME).ko $(MODULE_OUTPUT_DIR)/$(NETIF_MODULE_NAME).ko

clean::
	$(RM) $(BUILD_OUTPUT_DIR)

.PHONY: all compile install clean
.NOTPARALLEL: all compile install clean

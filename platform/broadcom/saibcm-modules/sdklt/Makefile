#
# $Copyright: Copyright 2018-2022 Broadcom. All rights reserved.
# The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License 
# version 2 as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# A copy of the GNU General Public License version 2 (GPLv2) can
# be found in the LICENSES folder.$
#
# Makefile for building Linux GPL kernel modules for SDKLT.
#

help:
	@echo ''
	@echo 'Build Linux GPL kernel modules for SDKLT.'
	@echo ''
	@echo 'Available make targets:'
	@echo 'kmod          - Build kernel modules'
	@echo 'clean         - Remove object files'
	@echo ''
	@echo 'Supported make variables:'
	@echo 'KDIR          - Linux kernel source directory (mandatory)'
	@echo 'CROSS_COPILE  - Cross-compiler prefix (optional)'
	@echo 'OUTPUT_DIR    - Output directory (build/ by default)'
	@echo ''
	@echo 'Examples:'
	@echo 'make -s KDIR=$$KERNEL/linux kmod'
	@echo 'make -s clean'
	@echo ''

ifndef KDIR
nokdir:
	@echo 'Error: The $$KDIR environment variable is not set.'
	@echo '$$KDIR must point to a configured Linux kernel source tree.'
	exit 1
endif

export KDIR
export CROSS_COMPILE

override SDK := $(CURDIR)

ifeq (,$(OUTPUT_DIR))
OUTPUT_DIR := $(CURDIR)/build
endif

BDE_BLDDIR := $(OUTPUT_DIR)/bde
KNET_BLDDIR := $(OUTPUT_DIR)/knet
KNETCB_BLDDIR := $(OUTPUT_DIR)/knetcb

LKM_CFLAGS += -UNGKNET_NETIF_MAX -DNGKNET_NETIF_MAX=1056 -UNGKNET_FILTER_MAX -DNGKNET_FILTER_MAX=1025
export LKM_CFLAGS

kmod: bde knet knetcb

bde:
	$(MAKE) -C $(SDK)/linux/bde SDK=$(SDK) \
		LKM_BLDDIR=$(BDE_BLDDIR) $(TARGET)
	ln -sf $(BDE_BLDDIR)/*.ko $(OUTPUT_DIR)

knet: bde
	$(MAKE) -C $(SDK)/linux/knet SDK=$(SDK) \
		KBUILD_EXTRA_SYMBOLS=$(BDE_BLDDIR)/Module.symvers \
		LKM_BLDDIR=$(KNET_BLDDIR) $(TARGET)
	ln -sf $(KNET_BLDDIR)/*.ko $(OUTPUT_DIR)

knetcb: knet
	$(MAKE) -C $(SDK)/linux/knetcb SDK=$(SDK) \
		KBUILD_EXTRA_SYMBOLS=$(KNET_BLDDIR)/Module.symvers \
		KBUILD_EXTRA_SYMBOLS+=$(PSAMPLE_SYMVERS)  \
		LKM_BLDDIR=$(KNETCB_BLDDIR) $(TARGET)
	ln -sf $(KNETCB_BLDDIR)/*.ko $(OUTPUT_DIR)

clean:
	$(MAKE) kmod TARGET=clean
	rm -f $(OUTPUT_DIR)/*.ko

distclean:
	rm -rf $(OUTPUT_DIR)

.PHONY: help kmod bde knet knetcb clean distclean

# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 1999 - 2026 Intel Corporation

src := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

KCOMPAT_KSRC := $(KERNEL_SRC)/source
KCOMPAT_KOBJ := $(KERNEL_SRC)/build
KCOMPAT_OUT := $(src)/kcompat_generated_defs.h
KCOMPAT_CONFIG_FILE := $(KCOMPAT_KOBJ)/include/generated/autoconf.h
PWD = $(shell pwd)

ccflags-y += -I$(src)

.PHONY: prepare all clean

prepare: $(KCOMPAT_OUT)

# kbuild part of makefile
#
# Makefile for the Intel(R) 10GbE PCI Express Linux Network Driver
#

obj-m += wb_ixgbe.o

define ixgbe-y
	ixgbe_main.o
	ixgbe_api.o
	ixgbe_common.o
	ixgbe_dcb.o
	ixgbe_dcb_82598.o
	ixgbe_dcb_82599.o
	ixgbe_ethtool.o
	ixgbe_lib.o
	ixgbe_mbx.o
	ixgbe_sriov.o
	ixgbe_param.o
	ixgbe_phy.o
	ixgbe_procfs.o
	ixgbe_xsk.o
	ixgbe_82598.o
	ixgbe_82599.o
	ixgbe_x540.o
	ixgbe_x550.o
	ixgbe_e610.o
	ixgbe_devlink.o
	ixgbe_fw_update.o
endef
wb_ixgbe-y := $(strip ${ixgbe-y})

wb_ixgbe-${CONFIG_DCB} += ixgbe_dcb_nl.o

wb_ixgbe-${CONFIG_DEBUG_FS} += ixgbe_debugfs.o

wb_ixgbe-${CONFIG_FCOE:m=y} += ixgbe_fcoe.o

wb_ixgbe-$(CONFIG_PTP_1588_CLOCK:m=y) += ixgbe_ptp.o
wb_ixgbe-$(CONFIG_PTP_1588_CLOCK:m=y) += ixgbe_ptp_e600.o

wb_ixgbe-${CONFIG_SYSFS} += ixgbe_sysfs.o

# Use kcompat pldmfw.c if kernel does not provide CONFIG_PLDMFW
ifndef CONFIG_PLDMFW
wb_ixgbe-y += kcompat_pldmfw.o
endif

wb_ixgbe-y += kcompat.o

# Force generation of kcompat_generated_defs.h before any ixgbe objects build
$(foreach f,$(wb_ixgbe-y) ixgbe_dcb_nl.o ixgbe_debugfs.o ixgbe_fcoe.o ixgbe_ptp.o ixgbe_ptp_e600.o ixgbe_sysfs.o kcompat_pldmfw.o, \
	$(eval $(obj)/$(f): $(KCOMPAT_OUT)))

all: prepare
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)

$(KCOMPAT_OUT): $(src)/kcompat-generator.sh $(KCOMPAT_CONFIG_FILE)
	@echo "  GEN     $@"
	@echo "  KSRC    $(KCOMPAT_KSRC)"
	@echo "  KOBJ    $(KCOMPAT_KOBJ)"
	@echo "  CONFIG  $(KCOMPAT_CONFIG_FILE)"
	@KSRC=$(KCOMPAT_KSRC) KOBJ=$(KCOMPAT_KOBJ) OUT=$(KCOMPAT_OUT) CONFIG_FILE=$(KCOMPAT_CONFIG_FILE) \
                bash $(src)/kcompat-generator.sh


clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd $(PWD)/*.order $(PWD)/*.mod $(PWD)/*.txt $(PWD)/*.symvers $(PWD)/kcompat_generated_defs.h

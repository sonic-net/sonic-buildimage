# -*- Makefile -*-
# $Id: Makefile,v 1.3 Broadcom SDK $
#
# $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
# 
# Permission is granted to use, copy, modify and/or distribute this
# software under either one of the licenses below.
# 
# License Option 1: GPL
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2, as
# published by the Free Software Foundation (the "GPL").
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License version 2 (GPLv2) for more details.
# 
# You should have received a copy of the GNU General Public License
# version 2 (GPLv2) along with this source code.
# 
# 
# License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
# 
# This software is governed by the Broadcom Open Network Switch APIs license:
# https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
# 
# 
#
LOCALDIR = systems/linux/kernel/modules/bcm-genl

include ${SDK}/make/Make.config

LIBS = $(LIBDIR)/libkern.a

ifndef DCBDIR
DCBDIR = $(SDK)/systems/linux/kernel/modules/dcb
# $(error 'The $$DCBDIR variable is not set')
endif
ifeq (1,$(NO_PRECOMPILED_MODULE))
$(error 'DCB_LIB build is not supported if NO_PRECOMPILED_MODULE=1')
endif
LIBS += $(LIBDIR)/libdcb.a
CFLAGS += -DDCBLIB_SUPPORT -I$(DCBDIR)/include
ifeq (1,$(BUILD_GENL_PACKET))
CFLAGS += -I${SDK}/systems/linux/kernel/modules/include/kernel
endif

ifdef LTSW_CHIPS
# bcmdrd_config.h is required for enum_max.h when LTSW devices are included
CFLAGS += -I${SDKLT}/bcmdrd/include
endif

KERNEL_MODULE_DIR = kernel_module

THIS_MOD_NAME := linux-bcm-genl
MODULE = $(LIBDIR)/$(THIS_MOD_NAME).o
KMODULE = $(LIBDIR)/$(THIS_MOD_NAME).ko

# BCM Network Device
# {
KBUILD_EXTRA_SYMBOLS := ${BLDDIR}/../bcm-knet/kernel_module/Module.symvers
ifeq (1,$(BUILD_GENL_PACKET))
KBUILD_EXTRA_SYMBOLS += ${BLDDIR}/../genl-packet/kernel_module/Module.symvers
endif
export KBUILD_EXTRA_SYMBOLS

build: $(MODULE) $(KMODULE)
#
# This is for the compilation of the 'precompiled' object. We keep it
# here for compatibility.
#
$(LIBDIR)/libdcb.a:
	$(MAKE) -C $(DCBDIR)/make BLDDIR=$(LIBDIR) \
		KADD_CFLAGS="-I${SDK}/systems/linux/kernel/modules/include $(KFLAGS) -D$(ENDIAN)" \
		CROSS_COMPILE=$(CROSS_COMPILE)

$(MODULE): $(BLDDIR)/.tree $(BOBJS) $(LIBS)
	$(LD) $(MODULE_LDFLAGS) -r -d $(BOBJS) $(LIBS) -o $@

$(KMODULE): $(MODULE)
	rm -fr $(BLDDIR)/$(KERNEL_MODULE_DIR)
	mkdir $(BLDDIR)/$(KERNEL_MODULE_DIR)
	cp ${SDK}/make/Makefile.linux-kmodule $(BLDDIR)/$(KERNEL_MODULE_DIR)/Makefile
	cat ${KBUILD_EXTRA_SYMBOLS} > $(BLDDIR)/$(KERNEL_MODULE_DIR)/Module.symvers
	MOD_NAME=$(THIS_MOD_NAME) $(MAKE) -C $(BLDDIR)/$(KERNEL_MODULE_DIR) $(THIS_MOD_NAME).ko
# }

# Make.depend is before clean:: so that Make.depend's clean:: runs first.

include ${SDK}/make/Make.depend

clean::
	$(RM) $(BLDDIR)/version.c $(BLDDIR)/version.o
	$(RM) $(BOBJS) $(MODULE)
	$(MAKE) -C $(DCBDIR)/make BLDDIR=$(LIBDIR) clean

.PHONY: build

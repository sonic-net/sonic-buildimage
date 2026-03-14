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
LOCALDIR = systems/linux/kernel/modules/knet-cb

include ${SDK}/make/Make.config

LIBS = $(LIBDIR)/libkern.a

ifeq (1,$(BUILD_DCBLIB))
ifndef DCBDIR
DCBDIR = $(SDK)/systems/linux/kernel/modules/dcb
# $(error 'The $$DCBDIR variable is not set')
endif
ifeq (1,$(NO_PRECOMPILED_MODULE))
$(error 'DCB_LIB build is not supported if NO_PRECOMPILED_MODULE=1')
endif
LIBS += $(LIBDIR)/libdcb.a
CFLAGS += -DDCBLIB_SUPPORT -I$(DCBDIR)/include
endif

KERNEL_MODULE_DIR = kernel_module

THIS_MOD_NAME := linux-knet-cb
MODULE = $(LIBDIR)/$(THIS_MOD_NAME).o
KMODULE = $(LIBDIR)/$(THIS_MOD_NAME).ko

# BCM Network Device

ifeq (1,$(NO_PRECOMPILED_MODULE))
# {
KBUILD_EXTRA_SYMBOLS := ${BLDDIR}/../bcm-knet/Module.symvers
export KBUILD_EXTRA_SYMBOLS
#
# These are the objects which need to be compiled, in the kernel, to
# created the module object file.
#
SRCS_COMPOSING = knet-cb.c ../shared/gmodule.c
OBJECTS_COMPOSING = "knet-cb.o gmodule.o"

build: $(KMODULE)
#
# Note that for NO_PRECOMPILED_MODULE, the subdirectory  'systems/linux/kernel/modules/knet-cb/kernel_module'
# is not created and all action is done in systems/linux/kernel/modules/knet-cb
#
$(KMODULE): $(SRCS_COMPOSING)
	mkdir -p $(BLDDIR)
	rm -fr $(BLDDIR)/$(KERNEL_MODULE_DIR)
	rm -f $(BLDDIR)/*.c
	rm -f $(BLDDIR)/*.o
	rm -f $(BLDDIR)/*.ko
	cp ${SDK}/make/Makefile.linux-kmodule $(BLDDIR)/Makefile
	cp ./*.c $(BLDDIR)/
	cp ../shared/*.c $(BLDDIR)/
	#
	# Wait until bcm-knet compilation is over so that we can copy its resultant
	# Module.symvers
	#
	if [ ! -f ${KBUILD_EXTRA_SYMBOLS} ]; then sleep 5; fi
	cat ${KBUILD_EXTRA_SYMBOLS} > $(BLDDIR)/Module.symvers
	MOD_OBJS=$(OBJECTS_COMPOSING) MOD_NAME=$(THIS_MOD_NAME) $(MAKE) -C $(BLDDIR) $(THIS_MOD_NAME).ko LOC_BLDDIR=$(BLDDIR) LOC_SRCDIR=$(PWD)
# }
else
# {
KBUILD_EXTRA_SYMBOLS := ${BLDDIR}/../bcm-knet/kernel_module/Module.symvers
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

# Make.depend is before clean:: so that Make.depend's clean:: runs first.
include ${SDK}/make/Make.depend
# }
endif

clean::
	$(RM) $(BLDDIR)/version.c $(BLDDIR)/version.o
	$(RM) $(BOBJS) $(MODULE)
ifeq (1,$(BUILD_DCBLIB))
	$(MAKE) -C $(DCBDIR)/make BLDDIR=$(LIBDIR) clean
endif

.PHONY: build

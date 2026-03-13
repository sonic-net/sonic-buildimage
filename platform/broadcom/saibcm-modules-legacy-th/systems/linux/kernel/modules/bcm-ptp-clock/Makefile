#
#  Copyright 2017 Broadcom
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License, version 2, as
#  published by the Free Software Foundation (the "GPL").
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License version 2 (GPLv2) for more details.
#
#  You should have received a copy of the GNU General Public License
#  version 2 (GPLv2) along with this source code.
#
# -*- Makefile -*-
# $Id: Makefile,v 1.3 2012/07/17 07:39:51 mlarsen Exp $
# $Copyright: (c) 2005 Broadcom Corp.
# All Rights Reserved.$
#
LOCALDIR = systems/linux/kernel/modules/bcm-ptp-clock

include ${SDK}/make/Make.config

LIBS = $(LIBDIR)/libkern.a

KERNEL_MODULE_DIR = kernel_module

THIS_MOD_NAME := linux-bcm-ptp-clock
MODULE = $(LIBDIR)/$(THIS_MOD_NAME).o
KMODULE = $(LIBDIR)/$(THIS_MOD_NAME).ko

ifeq (1,$(NO_PRECOMPILED_MODULE))
# {
BDE_SYMVERS := ${BLDDIR}/../../../../bde/linux/kernel/Module.symvers
KNET_SYMVERS := ${BLDDIR}/../bcm-knet/Module.symvers
KBUILD_EXTRA_SYMBOLS := $(BDE_SYMVERS) $(KNET_SYMVERS)
export KBUILD_EXTRA_SYMBOLS
#
# These are the objects which need to be compiled, in the kernel, to
# created the module object file.
#
SRCS_COMPOSING = bcm-ptp-clock.c ../shared/gmodule.c
OBJECTS_COMPOSING = "bcm-ptp-clock.o gmodule.o"

build: $(KMODULE)
#
# Note that for NO_PRECOMPILED_MODULE, the subdirectory 'systems/linux/kernel/modules/bcm-ptp-clock/kernel_module'
# is not created and all action is done in systems/linux/kernel/modules/bcm-ptp-clock
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
	if [ ! -f ${KNET_SYMVERS} ]; then sleep 5; fi
	cat ${KBUILD_EXTRA_SYMBOLS} > $(BLDDIR)/Module.symvers
	MOD_OBJS=$(OBJECTS_COMPOSING) MOD_NAME=$(THIS_MOD_NAME) $(MAKE) -C $(BLDDIR) $(THIS_MOD_NAME).ko LOC_BLDDIR=$(BLDDIR) LOC_SRCDIR=$(PWD)
# }
else
# {
KBUILD_EXTRA_SYMBOLS := ${BLDDIR}/../../../../bde/linux/kernel/kernel_module/Module.symvers
KBUILD_EXTRA_SYMBOLS += ${BLDDIR}/../bcm-knet/kernel_module/Module.symvers
export KBUILD_EXTRA_SYMBOLS

build: $(MODULE) $(KMODULE)

# BCM PTP Clock Device

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

ifneq ($(kernel_version),2_4)
.PHONY: build
endif

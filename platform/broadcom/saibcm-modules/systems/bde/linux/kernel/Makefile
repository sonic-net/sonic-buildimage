# -*- Makefile -*-
# $Id: Makefile,v 1.18 Broadcom SDK $
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
# Makefile for Linux kernel BDE
#
LOCALDIR = systems/bde/linux/kernel

# Make sure we build for the kernel if this is a user-mode build
ifneq ($(platform), )
override-target=linux-$(platform)
endif

include $(SDK)/make/Make.config

LIBS = $(LIBDIR)/libkern.a

# need to add vpath sources
VPATH = ../shared

# Add the srcs to be found by vpath
LSRCS += mpool.c

# Add shared BDE sources
VPATH += ../../shared
LSRCS += shbde_pci.c shbde_iproc.c shbde_mdio.c
CFLAGS += -I../../shared/include

ifeq (1,$(THREAD_SANITIZER_IS_ACTIVE))
# {
  $(warning ./systems/bde/linux/kernel/Makefile. Do not apply THREAD SANITIZER to kernel objects)
  override CFLAGS := $(subst -fsanitize=thread, ,$(CFLAGS))
# }
endif

LHDRS += mpool.h
LOBJS = $(addsuffix .o, $(basename $(LSRCS)))
BOBJS = $(addprefix $(BLDDIR)/,$(LOBJS))
#
# For 'NO_PRECOMPILED_MODULE', the first kernel compilation is done
# in systems/bde/linux/kernel (and not in systems/bde/linux/kernel/kernel_module,
# which is not even created).
# All the definitions above are for the first cycle of module compilation
#
PWD := $(shell pwd)

KERNEL_MODULE_DIR = kernel_module

THIS_MOD_NAME := linux-kernel-bde
MODULE = $(LIBDIR)/$(THIS_MOD_NAME).o
KMODULE = $(LIBDIR)/$(THIS_MOD_NAME).ko

build: $(KMODULE)

ifeq (1,$(NO_PRECOMPILED_MODULE))
# {
#
# These are the objects which need to be compiled, in the kernel, to
# created the module object file.
#
SRCS_COMPOSING = linux_shbde.c linux_dma.c ../shared/mpool.o $(THIS_MOD_NAME).c ../../shared/shbde_pci.o ../../shared/shbde_iproc.o ../../shared/shbde_mdio.o ../../../linux/kernel/modules/shared/gmodule.o
OBJECTS_COMPOSING = "linux_shbde.o linux_dma.o mpool.o $(THIS_MOD_NAME)-file.o shbde_pci.o shbde_iproc.o shbde_mdio.o gmodule.o"

module: $(KMODULE)

#
# Note that for NO_PRECOMPILED_MODULE, the subdirectory 'systems/bde/linux/kernel/kernel_module'
# is not created and all action is done in systems/bde/linux/kernel
#
$(KMODULE): $(SRCS_COMPOSING)
	mkdir -p $(BLDDIR)
	rm -fr $(BLDDIR)/$(KERNEL_MODULE_DIR)
	rm -f $(BLDDIR)/*.c
	rm -f $(BLDDIR)/*.o
	rm -f $(BLDDIR)/*.ko
	cp ${SDK}/make/Makefile.linux-kmodule $(BLDDIR)/Makefile
	#
	# Here we copy all source files, which are required for the
	# compilation of the module, into one directory. It seems that
	# specifying path for 'remote' files is not acceptable for
	# the module-creation mechanism in the kernel.
	#
	cp ./*.c $(BLDDIR)/
	cp ./*.h $(BLDDIR)/
	cp ../shared/*.c $(BLDDIR)/
	cp ../../shared/*.c $(BLDDIR)/
	cp ../../../linux/kernel/modules/shared/*.c $(BLDDIR)/
	#
	# We change here the name of the 'base' file since, it seems, the module-creation
	# mechanism does not add symbols when the name of one of the files is the same as
	# the name of the module itself.
	#
	mv $(BLDDIR)/$(THIS_MOD_NAME).c  $(BLDDIR)/$(THIS_MOD_NAME)-file.c
	MOD_OBJS=$(OBJECTS_COMPOSING) MOD_NAME=$(THIS_MOD_NAME) $(MAKE) -C $(BLDDIR) $(THIS_MOD_NAME).ko LOC_BLDDIR=$(BLDDIR) LOC_SRCDIR=$(PWD)
# }
else
# {
module: $(MODULE)
#
# This is for the compilation of the 'precompiled' object. We keep it
# here for compatibility.
#
$(MODULE): $(BLDDIR)/.tree $(BOBJS) kernel_libs
	mkdir -p $(@D)
	$(LD) $(MODULE_LDFLAGS) -r -d $(BOBJS) $(LIBS) -o $@
$(KMODULE): $(MODULE)
	rm -fr $(BLDDIR)/$(KERNEL_MODULE_DIR)
	mkdir $(BLDDIR)/$(KERNEL_MODULE_DIR)
	cp ${SDK}/make/Makefile.linux-kmodule $(BLDDIR)/$(KERNEL_MODULE_DIR)/Makefile
	MOD_NAME=$(THIS_MOD_NAME) $(MAKE) -C $(BLDDIR)/$(KERNEL_MODULE_DIR) $(THIS_MOD_NAME).ko

include $(SDK)/make/Make.depend
# }
endif

kernel_libs:
	$(MAKE) -C $(SDK)/systems/linux/kernel/modules/shared

# Make.depend is before clean:: so that Make.depend's clean:: runs first.

clean::
	$(MAKE) -C $(SDK)/systems/linux/kernel/modules/shared $@
	$(RM) $(BOBJS) $(MODULE)
	$(RM) $(BLDDIR)/$(KERNEL_MODULE_DIR)

distclean::

.PHONY: build kernel_libs

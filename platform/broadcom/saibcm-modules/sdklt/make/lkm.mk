#
# Copyright 2018-2025 Broadcom. All rights reserved.
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
# be found in the LICENSES folder.
#
# Shared makefile include for building Linux kernel modules.
#

# KDIR must point to the Linux kernel sources
ifndef KDIR
nokdir:; @echo 'The $$KDIR variable is not set'; exit 1
endif

# Required for older kernels
export EXTRA_CFLAGS = $(ccflags-y)

PWD := $(shell pwd)

ifneq ($(LKM_BLDDIR),)
#
# If a build directory has been specified, then we symlink all sources
# to this directory and redirect the module build path.
#
# Note that the KBUILD_OUTPUT variable cannot be used to redirect the
# output as we want it.
#
MDIR := $(LKM_BLDDIR)
MOBJS := $($(MOD_NAME)-y)
ifeq (,$(MOBJS))
MOBJS := $(obj-m)
endif
MSRCS := $(patsubst %.o,%.c,$(MOBJS))
MSRCS += Makefile Kbuild
BSRCS := $(addprefix $(PWD)/,$(MSRCS))
else
#
# Build in current directory by default.
#
MDIR := $(PWD)
endif

all: mlinks
	$(Q)echo Building kernel module $(MOD_NAME)
	$(MAKE) -C $(KDIR) M=$(MDIR)

clean:: mlinks
	$(Q)echo Cleaning kernel module $(MOD_NAME)
	$(MAKE) -C $(KDIR) M=$(MDIR) clean
ifneq ($(MDIR),$(PWD))
	rm -rf $(MDIR)
endif

mlinks:
ifneq ($(MDIR),$(PWD))
	$(Q)mkdir -p $(MDIR)
	(cd $(MDIR); \
	 rm -rf $(MSRCS); \
	 for f in $(BSRCS); do \
	     ln -s $$f; \
	 done)
endif

.PHONY: all mlinks clean

# Standard documentation targets
-include $(SDK)/make/doc.mk

#
# Copyright 2018-2024 Broadcom. All rights reserved.
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
# Linux KNET module.
#

CNETDIR = $(SDK)/bcmcnet
KNETDIR = $(SDK)/linux/knet
GENDIR = $(KNETDIR)/generated
SRCIDIR = $(CNETDIR)/include/bcmcnet
ifneq ($(OUTPUT_DIR),)
GENDIR = $(OUTPUT_DIR)/knet/generated
endif
DSTIDIR = $(GENDIR)/include/bcmcnet

# Change comma-separated list to space-separated list
comma = ,
empty =
space = $(empty) $(empty)
spc_sep = $(subst $(comma),$(space),$1)

# Convert chip name to uppercase
chip_uc = $(subst a,A,$(subst b,B,$(subst c,C,$(subst m,M,$1))))

# Convert chip name to lowercase
chip_lc = $(subst A,a,$(subst B,b,$(subst C,c,$(subst M,m,$1))))

#
# If SDK_CHIPS is defined, then exclude any chip directory which is
# not part of this list.
#
KNET_CHIPS := $(subst $(CNETDIR)/chip/,,$(wildcard $(CNETDIR)/chip/bcm*))
ifdef SDK_CHIPS
# Create space-separated lowercase version of chip list
SDK_CHIPS_SPC := $(call spc_sep,$(SDK_CHIPS))
SDK_CHIPS_LC := $(call chip_lc,$(SDK_CHIPS_SPC))
# Configure build flags according to chip list
KNET_CHIPS := $(filter $(SDK_CHIPS_LC),$(KNET_CHIPS))
KNET_CPPFLAGS := CHIP_DEFAULT=0 $(addsuffix =1,$(call chip_uc,$(KNET_CHIPS)))
KNET_CPPFLAGS := $(addprefix -DBCMDRD_CONFIG_INCLUDE_,$(KNET_CPPFLAGS))
export KNET_CPPFLAGS
endif

.PHONY: mklinks rmlinks

knet: mklinks
	$(MAKE) -C $(GENDIR) all

#
# Suppress symlink error messages.
#
# Note that we do not use "ln -f" as this may cause failures if
# multiple builds are done in parallel on the same source tree.
#
R = 2>/dev/null

mklinks:
	mkdir -p $(DSTIDIR)
	-ln -sr $(KNETDIR)/ngknet_dep.h $(DSTIDIR)/bcmcnet_dep.h $(R)
	-ln -sr $(KNETDIR)/ngknet_buff.h $(DSTIDIR)/bcmcnet_buff.h $(R)
	-ln -sr $(SRCIDIR)/bcmcnet_types.h $(DSTIDIR) $(R)
	-ln -sr $(SRCIDIR)/bcmcnet_internal.h $(DSTIDIR) $(R)
	-ln -sr $(SRCIDIR)/bcmcnet_core.h $(DSTIDIR) $(R)
	-ln -sr $(SRCIDIR)/bcmcnet_dev.h $(DSTIDIR) $(R)
	-ln -sr $(SRCIDIR)/bcmcnet_rxtx.h $(DSTIDIR) $(R)
	-ln -sr $(SRCIDIR)/bcmcnet_cmicd.h $(DSTIDIR) $(R)
	-ln -sr $(SRCIDIR)/bcmcnet_cmicx.h $(DSTIDIR) $(R)
	-ln -sr $(SRCIDIR)/bcmcnet_cmicr_acc.h $(DSTIDIR) $(R)
	-ln -sr $(SRCIDIR)/bcmcnet_cmicr.h $(DSTIDIR) $(R)
	-ln -sr $(SRCIDIR)/bcmcnet_cmicr2.h $(DSTIDIR) $(R)
	-ln -sr $(CNETDIR)/chip/*/*attach.c $(GENDIR) $(R)
	-ln -sr $(CNETDIR)/hmi/cmicd/*.c $(GENDIR) $(R)
	-ln -sr $(CNETDIR)/hmi/cmicx/*.c $(GENDIR) $(R)
	-ln -sr $(CNETDIR)/hmi/cmicr/*.c $(GENDIR) $(R)
	-ln -sr $(CNETDIR)/main/bcmcnet_core.c $(GENDIR) $(R)
	-ln -sr $(CNETDIR)/main/bcmcnet_dev.c $(GENDIR) $(R)
	-ln -sr $(CNETDIR)/main/bcmcnet_rxtx.c $(GENDIR) $(R)
	-ln -sr $(KNETDIR)/*.[ch] $(GENDIR) $(R)
	-ln -sr $(KNETDIR)/Makefile $(GENDIR) $(R)
	-ln -sr $(KNETDIR)/Kbuild $(GENDIR) $(R)

rmlinks:
	-rm -rf $(GENDIR)

CHIP_SRCS := $(addsuffix _pdma_attach.c,$(KNET_CHIPS))
CHIP_OBJS ?= $(patsubst %.c, %.o, $(CHIP_SRCS))
export CHIP_OBJS

include Kbuild

ifeq ($(KERNELRELEASE),)

MOD_NAME = linux_ngknet

include $(SDK)/make/lkm.mk

endif

.PHONY: distclean

distclean:: rmlinks

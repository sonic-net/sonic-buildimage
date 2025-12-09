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
# Helper makefile for building Linux kernel module that depends on the
# SDK Packet Meta Data (PMD) library.
#
# The makefile provides a make target named 'kpmd', which the main
# kernel module makefile should depend on.
#
# The 'kpmd' make target will create symbolic links from the relevant
# SDK source files into the module source directory, which must be
# specified via either one of $(KMODDIR) or $(GENDIR).
#
# The 'kpmd' make target also exports two make variables, of which
# $(SDK_PMD_KFLAGS) should be added to the kernel module build flags,
# and $(SDK_PMD_KOBJS) should be added to the list of kernel module
# object files.
#
# For example usage, please refer to $SDK/linux/bcmgenl/Makefile.
#

ifndef SDK
$(error The $$SDK environment variable is not set)
endif

# SDK make utilities
include $(SDK)/make/makeutils.mk

# SDK source directories
SHRDIR = $(SDK)/shr
BCMPKTDIR = $(SDK)/bcmpkt
BCMPKTIDIR = $(BCMPKTDIR)/include/bcmpkt

# Create links locally if no GENDIR was specified
ifeq (,$(GENDIR))
GENDIR = $(KMODDIR)
endif

#
# Suppress symlink error messages.
#
# Note that we do not use "ln -f" as this may cause failures if
# multiple builds are done in parallel on the same source tree.
#
R = 2>/dev/null

mklinks: config
	mkdir -p $(GENDIR)
	-ln -s $(BCMPKTDIR)/chip/*/*lbhdr.c $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/chip/*/*rxpmd.c $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/chip/*/*rxpmd_field.c $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/chip/*/*txpmd.c $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/lbpmd/bcmpkt_lbhdr.c $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/rxpmd/bcmpkt_rxpmd.c $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/rxpmd/bcmpkt_rxpmd_match_id.c $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/txpmd/bcmpkt_txpmd.c $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/flexhdr/bcmpkt_flexhdr.c $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/include/bcmpkt/bcmpkt_flexhdr_field.h $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/include/bcmpkt/bcmpkt_rxpmd_match_id_defs.h $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/xfcr/*/*/*.c $(GENDIR) $(R)
	-ln -s $(BCMPKTDIR)/ltt_stub/*/*/*/*.c $(GENDIR) $(R)
	-ln -s $(SHRDIR)/bitop/shr_bitop_range_clear.c $(GENDIR) $(R)
	-ln -s $(KMODDIR)/*.[ch] $(GENDIR) $(R)
	-ln -s $(KMODDIR)/Makefile $(GENDIR) $(R)
	-ln -s $(KMODDIR)/Kbuild $(GENDIR) $(R)

rmlinks:
	-rm -f $(GENDIR)/bcm*
	-rm -f $(GENDIR)/shr*

# FLTG tools directory (not present in GPL package)
FLTG_DIR := $(SDK)/tools/fltg

# GPL release does not contain FLTG tools
ifneq (,$(wildcard $(FLTG_DIR)))
HAS_FLTG := 1
endif

# File indicating that the FLTG build is complete
FLTG_DONE := $(FLTG_DIR)/generated/ltt.sum

# If not GPL, check that FLTG files have been generated
config:
ifeq (1,$(HAS_FLTG))
	@echo Updating SDK configuration for PMD library
	$(MAKE) -C $(SDK) config
endif

# If not GPL, extract default variant name
ifeq (1,$(HAS_FLTG))
include $(SDK)/make/defvar.mk
endif

kpmd: mklinks

distclean:: rmlinks

.PHONY: mklinks rmlinks config kpmd distclean

ALL_CHIPS := $(subst $(BCMPKTDIR)/chip/,,$(wildcard $(BCMPKTDIR)/chip/bcm*))
VAR_CHIPS := $(subst $(BCMPKTDIR)/xfcr/,,$(wildcard $(BCMPKTDIR)/xfcr/bcm*))

# Set options for partial build support. Note that this will define
# both SDK_CHIPS_UC and SDK_VARIANTS_UC, which are used below.
include $(SDK)/make/partial.mk

# Set PMD_CHIPS and VARIANT_DIRS
ifdef SDK_CHIPS
PMD_CHIPS := $(call var_lc,$(SDK_CHIPS_UC))
ifdef SDK_VARIANTS
# Both SDK_CHIPS and SDK_VARIANTS
SDK_VARIANTS_LC := $(call var_lc,$(SDK_VARIANTS_UC))
TMP_ALL_VAR_DIRS = $(foreach K, $(PMD_CHIPS),$(filter-out $(SDK_VARIANTS_LC),\
	$(wildcard $(BCMPKTDIR)/xfcr/$(K)/*)))
VARIANT_DIRS := $(foreach K, $(PMD_CHIPS),$(foreach V, $(SDK_VARIANTS_LC),\
	$(findstring $(BCMPKTDIR)/xfcr/$K/$V,$(TMP_ALL_VAR_DIRS))))
else
# SDK_CHIPS only
VARIANT_DIRS := $(foreach K, $(PMD_CHIPS),\
	$(wildcard $(BCMPKTDIR)/xfcr/$(K)/* -type d))
# If SDK_VARIANTS is not defined but SDK_CHIPS is defined, we want all
# variants for the chips so set SDK_VARIANTS for the partial build to
# work correctly.
SDK_VARIANTS_SPC := $(foreach D, $(VARIANT_DIRS),$(lastword $(subst /, ,$D)))
SDK_VARIANTS_LC := $(call var_lc,$(SDK_VARIANTS_SPC))
SDK_VARIANTS := $(SDK_VARIANTS_LC)
endif # SDK_VARIANTS
else
# Neither SDK_VARIANTS or SDK_CHIPS
PMD_CHIPS := $(ALL_CHIPS)
VARIANT_DIRS := $(foreach K, $(filter $(VAR_CHIPS),$(PMD_CHIPS)),\
	$(wildcard $(BCMPKTDIR)/xfcr/$(K)/* -type d))
endif # SDK_CHIPS

PMD_FLEX_CHIPS := $(filter $(PMD_CHIPS),$(sort $(foreach D, $(VARIANT_DIRS), \
	$(lastword $(filter-out $(lastword $(subst /, ,$D)),$(subst /, ,$D))))))

CHIP_SRCS := $(addsuffix _pkt_lbhdr.c,$(PMD_CHIPS))
CHIP_SRCS += $(addsuffix _pkt_rxpmd.c,$(PMD_CHIPS))
ifneq (,$(PMD_FLEX_CHIPS))
CHIP_SRCS += $(addsuffix _pkt_rxpmd_field.c,$(PMD_FLEX_CHIPS))
endif
CHIP_SRCS += $(addsuffix _pkt_txpmd.c,$(PMD_CHIPS))

VARIANTS := $(subst /,_, $(subst $(BCMPKTDIR)/xfcr/,,$(sort $(VARIANT_DIRS))))
CHIP_SRCS += $(addsuffix _pkt_flexhdr.c,$(VARIANTS))
CHIP_SRCS += $(addsuffix _bcmpkt_rxpmd_match_id.c,$(VARIANTS))

ifneq (,$(wildcard $(BCMPKTDIR)/ltt_stub/*))
STUB_DIRS := $(sort $(shell find $(BCMPKTDIR)/ltt_stub -mindepth 3 -type d))
endif
ifneq (,$(STUB_DIRS))
STUB_VARS := $(subst /,_, $(subst $(BCMPKTDIR)/ltt_stub/generated/,,$(sort $(STUB_DIRS))))
CHIP_SRCS += $(addsuffix _pkt_flexhdr.c,$(STUB_VARS))
CHIP_SRCS += $(addsuffix _bcmpkt_rxpmd_match_id.c,$(STUB_VARS))
endif

CHIP_OBJS ?= $(patsubst %.c, %.o, $(CHIP_SRCS))

SDK_PMD_KFLAGS := -DSAL_LINUX -DKPMD $(SDK_CPPFLAGS) \
		  -I$(SDK)/sal/include \
		  -I$(SDK)/bcmltd/include \
		  -I$(SDK)/bcmlrd/include \
		  -I$(SDK)/bcmdrd/include \
		  -I$(SDK)/bcmpkt/include
export SDK_PMD_KFLAGS

COMMON_SRCS := bcmpkt_lbhdr.c
COMMON_SRCS += bcmpkt_rxpmd.c
COMMON_SRCS += bcmpkt_rxpmd_match_id.c
COMMON_SRCS += bcmpkt_txpmd.c
COMMON_SRCS += bcmpkt_flexhdr.c
COMMON_SRCS += shr_bitop_range_clear.c

SDK_PMD_KOBJS ?= $(patsubst %.c, %.o, $(COMMON_SRCS) $(CHIP_SRCS))
export SDK_PMD_KOBJS

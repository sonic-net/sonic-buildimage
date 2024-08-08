#
# $Copyright: Copyright 2018-2023 Broadcom. All rights reserved.
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
# Helper makefile for building stand-alone PMD kernel module
#

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

# Check for valid FLTG configuration by default
ifneq (0,$(KPMD_CONFIG_CHECK))
KPMD_CONFIG := config
endif

mklinks: $(KPMD_CONFIG)
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
	-rm -f $(KMODDIR)/bcm*
	-rm -f $(KMODDIR)/shr*

# FLTG tools directory (not present in GPL package)
FLTG_DIR := $(SDK)/tools/fltg

# File indicating that the FLTG build is complete
FLTG_DONE := $(FLTG_DIR)/generated/ltt.sum

# If not GPL, check that FLTG files have been generated
config:
	if [ -d $(FLTG_DIR) ]; then \
	    if [ ! -e $(FLTG_DONE) ]; then \
	        echo 'kpmd.mk: Please run "make -C $$SDK config"' \
	             'before building the Linux PMD library'; \
	        exit 1; \
	    fi \
	fi

kpmd: mklinks

distclean:: rmlinks

.PHONY: mklinks rmlinks config kpmd distclean

ALL_CHIPS := $(subst $(BCMPKTDIR)/chip/,,$(wildcard $(BCMPKTDIR)/chip/bcm*))
VAR_CHIPS := $(subst $(BCMPKTDIR)/xfcr/,,$(wildcard $(BCMPKTDIR)/xfcr/bcm*))

# If SDK_VARIANTS is defined but not SDK_CHIPS, find the chips for the
# specified variants and set SDK_CHIPS so the partial build can work correctly
ifdef SDK_VARIANTS
SDK_VARIANTS_SPC := $(call spc_sep,$(SDK_VARIANTS))
SDK_VARIANTS_LC := $(call var_lc,$(SDK_VARIANTS_SPC))
ifdef SDK_CHIPS
# Both SDK_CHIPS and SDK_VARIANTS
# Set PMD_CHIPS and VARIANT_DIRS
SDK_CHIPS_SPC := $(call spc_sep,$(SDK_CHIPS))
SDK_CHIPS_LC := $(call var_lc,$(SDK_CHIPS_SPC))
PMD_CHIPS := $(SDK_CHIPS_LC)
TMP_ALL_VAR_DIRS = $(foreach K, $(PMD_CHIPS),$(filter-out $(SDK_VARIANTS_LC),\
	$(shell find $(BCMPKTDIR)/xfcr/$K/* -type d)))
VARIANT_DIRS := $(foreach K, $(SDK_CHIPS_LC),$(foreach V, $(SDK_VARIANTS_LC),\
	$(findstring $(BCMPKTDIR)/xfcr/$K/$V,$(TMP_ALL_VAR_DIRS))))
else
# SDK_VARIANTS only
# Set SDK_CHIPS, PMD_CHIPS and VARIANT_DIRS
TMP_ALL_VAR_DIRS = $(foreach K, $(ALL_CHIPS),$(filter-out $(SDK_VARIANTS_LC),\
	$(shell find $(BCMPKTDIR)/xfcr/$K/* -type d)))
VARIANT_DIRS := $(foreach K, $(ALL_CHIPS),$(foreach V, $(SDK_VARIANTS_LC),\
	$(findstring $(BCMPKTDIR)/xfcr/$K/$V,$(TMP_ALL_VAR_DIRS))))
TMP_VARIANTS = $(foreach D, $(VARIANT_DIRS),$(lastword $(subst /, ,$D)))
SDK_CHIPS_SPC := $(foreach D, $(VARIANT_DIRS),$(lastword $(filter-out \
	$(lastword $(subst /, ,$D)),$(subst /, ,$D))))
SDK_CHIPS_LC := $(call var_lc,$(SDK_CHIPS_SPC))
SDK_CHIPS := $(SDK_CHIPS_LC)
PMD_CHIPS := $(SDK_CHIPS)
endif
else
# If SDK_VARIANTS is not defined but SDK_CHIPS is defined, we want all variants
# for the chips so set SDK_VARIANTS for the partial build to work correctly
ifdef SDK_CHIPS
# SDK_CHIPS only
# Set SDK_VARIANTS, PMD_CHIPS and VARIANT_DIRS
SDK_CHIPS_SPC := $(call spc_sep,$(SDK_CHIPS))
SDK_CHIPS_LC := $(call var_lc,$(SDK_CHIPS_SPC))
VARIANT_DIRS := $(foreach K, $(SDK_CHIPS),\
	$(shell find $(BCMPKTDIR)/xfcr/$K/* -type d))
SDK_VARIANTS_SPC := $(foreach D, $(VARIANT_DIRS),$(lastword $(subst /, ,$D)))
SDK_VARIANTS_LC := $(call var_lc,$(SDK_VARIANTS_SPC))
SDK_VARIANTS := $(SDK_VARIANTS_LC)
PMD_CHIPS := $(SDK_CHIPS_LC)
else
# Neither SDK_VARIANTS or SDK_CHIPS
# Set PMD_CHIPS and VARIANT_DIRS
PMD_CHIPS := $(ALL_CHIPS)
VARIANT_DIRS := $(foreach K, $(filter $(VAR_CHIPS),$(PMD_CHIPS)),\
	$(shell find $(BCMPKTDIR)/xfcr/$K/* -type d))
endif
endif

# Set options for partial build support
include $(SDK)/make/partial.mk

ifdef SDK_CHIPS
KNETCB_CPPFLAGS := $(SDK_CPPFLAGS)
endif

ifdef SDK_VARIANTS
override KNETCB_CPPFLAGS := $(SDK_CPPFLAGS)
endif

KNETCB_CPPFLAGS += -DKPMD
export KNETCB_CPPFLAGS

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

SDK_PMD_KFLAGS := -DSAL_LINUX \
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

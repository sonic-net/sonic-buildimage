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
# SDK partial build support
#

include $(SDK)/make/makeutils.mk

#
# If SDK_CHIPS is defined, then exclude any chip which is not part of
# this list. Note that bond-option chips must be added via SDK_SKUS
# separately if needed.
#
ifneq (,$(SDK_CHIPS))
# Create space-separated uppercase version of chip list
SDK_CHIPS_SPC := $(call spc_sep,$(SDK_CHIPS))
SDK_CHIPS_UC := $(call chip_uc,$(SDK_CHIPS_SPC))
endif
ifneq (,$(SDK_SKUS))
# Create space-separated uppercase version of SKU list
SDK_SKUS_SPC := $(call spc_sep,$(SDK_SKUS))
SDK_SKUS_UC := $(call chip_uc,$(SDK_SKUS_SPC))
endif

#
# If SDK_VARIANTS is defined, then exclude any chip variant which is
# not part of this list.
#
ifneq (,$(SDK_VARIANTS))
# Create space-separated uppercase version of chip variant list
SDK_VARIANTS_SPC := $(call spc_sep,$(SDK_VARIANTS))
SDK_VARIANTS_UC := $(call var_uc,$(SDK_VARIANTS_SPC))
endif

ifneq (,$(SDK_CHIPS))
CHIP_CPPFLAGS := CHIP_DEFAULT=0 $(addsuffix =1,$(SDK_CHIPS_UC) $(SDK_SKUS_UC))
CHIP_CPPFLAGS := $(addprefix -DBCMDRD_CONFIG_INCLUDE_,$(CHIP_CPPFLAGS))
TMP_CPPFLAGS := $(filter-out $(CHIP_CPPFLAGS),$(SDK_CPPFLAGS))
override SDK_CPPFLAGS := $(TMP_CPPFLAGS) $(CHIP_CPPFLAGS)
ifdef SDK_VARIANTS
VAR_CPPFLAGS := VARIANT_DEFAULT=0
VAR_CPPFLAGS += $(foreach C,$(SDK_CHIPS_UC),$(addprefix $(C)_,$(addsuffix =1,$(SDK_VARIANTS_UC))))
VAR_CPPFLAGS := $(addprefix -DBCMLTD_CONFIG_INCLUDE_,$(VAR_CPPFLAGS))
TMP_CPPFLAGS := $(filter-out $(VAR_CPPFLAGS),$(SDK_CPPFLAGS))
override SDK_CPPFLAGS := $(TMP_CPPFLAGS) $(VAR_CPPFLAGS)
endif # SDK_VARIANTS
else
# If SDK_VARIANTS is defined, but SDK_CHIPS is not.
ifneq (,$(SDK_VARIANTS))
$(error 'Specifying SDK_VARIANTS requires a non-empty SDK_CHIPS')
endif # SDK_VARIANTS
endif # SDK_CHIPS

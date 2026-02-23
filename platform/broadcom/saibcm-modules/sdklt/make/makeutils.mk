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
# Various make utility functions.
#

ifneq (1,MAKEUTILS)
MAKEUTILS := 1

# Change comma-separated list to space-separated list
comma = ,
empty =
space = $(empty) $(empty)
spc_sep = $(subst $(comma),$(space),$1)
comma_sep = $(subst $(space),$(comma),$1)

# Convert chip name to uppercase
chip_uc = $(subst a,A,$(subst b,B,$(subst c,C,$(subst m,M,$1))))

# Convert chip name to lowercase
chip_lc = $(subst A,a,$(subst B,b,$(subst C,c,$(subst M,m,$1))))

# Convert chip variant name to uppercase
var_uc = $(shell echo $1 | tr a-z A-Z)

# Convert chip variant name to lowercase
var_lc = $(shell echo $1 | tr A-Z a-z)

endif

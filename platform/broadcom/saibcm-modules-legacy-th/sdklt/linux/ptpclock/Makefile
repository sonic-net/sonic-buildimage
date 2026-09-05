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
# Linux PTP clock module.
#

include Kbuild

ifeq ($(KERNELRELEASE),)

MOD_NAME = linux_ngptpclock

include $(SDK)/make/lkm.mk

endif

.PHONY: distclean

distclean::

# -*- Makefile -*-
# $Id: Makefile,v 1.2 Broadcom SDK $
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
LOCALDIR = systems/linux/kernel/modules/shared

include ${SDK}/make/Make.config

ifeq (1,$(THREAD_SANITIZER_IS_ACTIVE))
# {
  $(warning ./systems/linux/kernel/modules/shared/Makefile. Do not apply THREAD SANITIZER to kernel objects)
  override CFLAGS := $(subst -fsanitize=thread, ,$(CFLAGS))
# }
endif

lib = libkern

include ${SDK}/make/Make.kernlib

include ${SDK}/make/Make.depend

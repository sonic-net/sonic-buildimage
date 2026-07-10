################################################################################
# Copyright (C) 2020  MediaTek, Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# version 2 along with this program.
################################################################################
DEV_MODULE_NAME            := nps_dev
NETIF_MODULE_NAME          := nps_netif
################################################################################
DEV_OBJS_TOTAL             := ./src/osal_mdc.o ./src/osal_isymbol.o
NETIF_OBJS_TOTAL           := ./src/hal_tau_pkt_knl.o ./src/netif_perf.o ./src/netif_osal.o ./src/netif_nl.o

obj-m                      := $(DEV_MODULE_NAME).o $(NETIF_MODULE_NAME).o
$(DEV_MODULE_NAME)-objs    := $(DEV_OBJS_TOTAL)
$(NETIF_MODULE_NAME)-objs  := $(NETIF_OBJS_TOTAL)

KBUILD_EXTRA_SYMBOLS       := $(BUILD_OUTPUT_DIR)/Module.symvers
################################################################################
folder:
	$(TEST_PATH) $(BUILD_OUTPUT_DIR) || $(MKDIR) $(BUILD_OUTPUT_DIR)
	$(TEST_PATH) $(BUILD_OUTPUT_DIR)/src || $(MKDIR) $(BUILD_OUTPUT_DIR)/src

compile:: folder
	touch $(BUILD_OUTPUT_DIR)/Makefile
	$(MAKE) -C $(OS_PATH) M=$(BUILD_OUTPUT_DIR) src=$(shell pwd) modules EXTRA_CFLAGS="$(EXTRA_CFLAGS)" KBUILD_EXTRA_SYMBOLS=$(KBUILD_EXTRA_SYMBOLS)
install::

clean::

################################################################################
# Copyright (C) 2026  Grandtrans, Inc.
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
NETIF_MODULE_NAME          := gt_netif
################################################################################
NETIF_OBJS_TOTAL           := src/gt_netif/netif_knl.o \
                              src/gt_netif/netif_osal.o \
                              src/gt_netif/netif_dev.o  \
                              src/gt_netif/shm_netdev.o

################################################################################
GT_DEV_MODULE_NAME         := gt_dev
GT_DEV_OBJS_TOTAL          := src/gt_dev/gt_drv_kernel.o

################################################################################
folder:
	$(TEST_PATH) $(BUILD_OUTPUT_DIR) || $(MKDIR) $(BUILD_OUTPUT_DIR)

compile:: folder
	@echo "obj-m := $(NETIF_MODULE_NAME).o $(GT_DEV_MODULE_NAME).o" > $(BUILD_OUTPUT_DIR)/Makefile
	@echo "$(NETIF_MODULE_NAME)-objs := $(addprefix ../, $(NETIF_OBJS_TOTAL))" >> $(BUILD_OUTPUT_DIR)/Makefile
	@echo "$(GT_DEV_MODULE_NAME)-objs := $(addprefix ../, $(GT_DEV_OBJS_TOTAL))" >> $(BUILD_OUTPUT_DIR)/Makefile
	$(MAKE) -C $(OS_PATH) M=$(BUILD_OUTPUT_DIR) modules \
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)"

install::

clean::

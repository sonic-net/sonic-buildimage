KVERSION   ?= $(shell uname -r)
KERNEL_MAJOR_MINOR := $(shell echo $(KVERSION) | awk -F. '{print $$1"."$$2}')
VERSION_THRESHOLD := 6.11
NEW_VER := $(shell echo $(KERNEL_MAJOR_MINOR) | tr -d .)
THRESHOLD_VER := $(shell echo $(VERSION_THRESHOLD) | tr -d .)

MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
MODULES_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/modules)
FIRMWARE_UPGRADE_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/app/firmware_upgrade/firmware_driver/include)

EXTRA_CFLAGS+= -I$(MODULES_DIR)
EXTRA_CFLAGS+= -I$(FIRMWARE_UPGRADE_PATH)

obj-m := mc_cpld.o
obj-m += fpga_pcie_uart.o
obj-m += wb_i2c_mux_pca954x_device.o
obj-m += wb_pcie_dev_device.o
# obj-m += wb_firmware_upgrade_device.o
obj-m += wb_i2c_dev_device.o

# 版本比较：小于 6.11 用旧目录，否则用新目录
ifeq ($(shell test $(NEW_VER) -lt $(THRESHOLD_VER); echo $$?),0)
	obj-y += linux-6.1/wb_amd_xgbe/
else
	obj-y += linux-6.12/wb_amd_xgbe/
endif

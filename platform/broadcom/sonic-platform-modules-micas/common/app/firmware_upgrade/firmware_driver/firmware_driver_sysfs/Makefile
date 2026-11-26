#include $(top_srcdir)/debian/rules
#KERNELDIR := ${KBUILD_OUTPUT}

PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
FIRMWARE_UPGRADE_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../include)
EXTRA_CFLAGS+= -I$(FIRMWARE_UPGRADE_PATH)
EXTRA_CFLAGS+= -Wall

firmware_driver_sysfs-objs := firmware.o
firmware_driver_sysfs-objs += firmware_sysfs.o firmware_sysfs_upgrade.o

#ifndef CONFIG_FRM_PRODUCT_FILE

$(warning $(firmware_driver_sysfs-objs))
obj-m := firmware_driver_sysfs.o
all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(common_module_dir) ]; then mkdir -p $(common_module_dir) ;fi
	cp -p  $(PWD)/*.ko $(common_module_dir)

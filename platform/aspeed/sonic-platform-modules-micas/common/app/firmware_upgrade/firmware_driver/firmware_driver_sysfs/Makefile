#include $(top_srcdir)/debian/rules
#KERNELDIR := ${KBUILD_OUTPUT}

MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_I_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../include)
FIRMWARE_UPGRADE_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../include)
FIRMWARE_UPGRADE_PUBLIC_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../../include)

EXTRA_CFLAGS:= -I$(MAKEFILE_I_PATH)
EXTRA_CFLAGS+= -I$(FIRMWARE_UPGRADE_PATH) -I$(FIRMWARE_UPGRADE_PUBLIC_PATH)
EXTRA_CFLAGS+= -Wall

firmware_driver_sysfs-objs := firmware.o
firmware_driver_sysfs-objs += firmware_sysfs.o firmware_sysfs_upgrade.o

#ifndef CONFIG_FRM_PRODUCT_FILE

$(warning $(firmware_driver_sysfs-objs))
obj-m += firmware_driver_sysfs.o
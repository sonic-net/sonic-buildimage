#include $(top_srcdir)/debian/rules
#KERNELDIR := ${KBUILD_OUTPUT}

PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
FIRMWARE_UPGRADE_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../include)
EXTRA_CFLAGS+= -I$(FIRMWARE_UPGRADE_PATH)
EXTRA_CFLAGS+= -Wall

firmware_driver_cpld-objs := firmware.o
firmware_driver_cpld-objs += firmware_cpld.o firmware_cpld_upgrade.o
firmware_driver_cpld-objs += jbicomp.o jbijtag.o jbimain.o jbistub.o

#ifndef CONFIG_FRM_PRODUCT_FILE

$(warning $(firmware_driver_cpld-objs))
obj-m := firmware_driver_cpld.o
all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(common_module_dir) ]; then mkdir -p $(common_module_dir) ;fi
	cp -p  $(PWD)/*.ko $(common_module_dir)

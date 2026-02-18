#include $(top_srcdir)/debian/rules
#KERNELDIR := ${KBUILD_OUTPUT}

PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
FIRMWARE_UPGRADE_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../../include)
BSP_KERNEL_DEBUG_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../../../../../public/include)
EXTRA_CFLAGS+= -I$(FIRMWARE_UPGRADE_PATH) -I$(BSP_KERNEL_DEBUG_PATH)
EXTRA_CFLAGS+= -Wall

firmware_driver_ispvme_i2c-objs := firmware_ispvme_i2c.o

$(warning $(firmware_driver_ispvme_i2c-objs))
obj-m := firmware_driver_ispvme_i2c.o

all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(common_module_dir) ]; then mkdir -p $(common_module_dir) ;fi
	cp -p  $(PWD)/*.ko $(common_module_dir)
#include $(top_srcdir)/debian/rules
#KERNELDIR := ${KBUILD_OUTPUT}

INCLUDE_PATH := $(src)/../../include
PUBLIC_INCLUDE_PATH := $(src)/../../../../public/include

ccflags-y += -I$(INCLUDE_PATH)
ccflags-y += -I$(PUBLIC_INCLUDE_PATH)

firmware_driver_ispvme_i2c-objs := firmware_ispvme_i2c.o

#ifndef CONFIG_FRM_PRODUCT_FILE

$(warning $(firmware_driver_ispvme_i2c-objs))
obj-m += firmware_driver_ispvme_i2c.o

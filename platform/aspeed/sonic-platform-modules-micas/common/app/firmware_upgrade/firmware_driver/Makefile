SRC = $(shell pwd)

EXTRA_CFLAGS := -Wall
MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BSP_KERNEL_DEBUG_PATH = $(abspath $(MAKEFILE_DIR)/../../../public/include)
subdir-ccflags-y += -Wall -I$(BSP_KERNEL_DEBUG_PATH)

obj-y += firmware_driver_ispvme/
obj-y += firmware_driver_cpld/
obj-y += firmware_driver_sysfs/
obj-y += firmware_driver_ispvme_i2c/



all:
	echo "subdir-ccflags-y = ${subdir-ccflags-y} MAKEFILE_DIR=${MAKEFILE_DIR} "
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules

install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean

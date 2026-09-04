# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Ciena Common Platform Modules
# These modules are shared across Ciena platforms
#

# Kernel build environment
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build

# Extra compiler flags
EXTRA_CFLAGS += -DNO_IRQ=0
EXTRA_CFLAGS += -I$(src)
EXTRA_CFLAGS += -I$(src)/raw_chardev
EXTRA_CFLAGS += -I$(src)/gnss
EXTRA_CFLAGS += -include $(src)/kcompat.h
EXTRA_CFLAGS += -Wall -Wno-unused-parameter

# Common kernel modules (shared across all platforms)
obj-m += i2c-ciena.o
obj-m += i2c-ciena-smb.o
obj-m += i2c-fpga-mux.o
obj-m += ciena_cic_trace.o
obj-m += ciena_thermal.o
obj-m += ciena_fan.o
obj-m += gpiotest-sysfs.o
obj-m += led-sysfs.o
obj-m += regmap-sysfs.o
obj-m += reset-fpga.o
obj-m += reset-gpio.o
obj-m += reset-ipmi.o
obj-m += reset-sysfs.o
obj-m += ciena_fpga_watchdog.o
# Note: sirilx_gpio and sirilx_uio need platform-specific defines (GPIO_NAME, MOD_NAME, etc.)
# They should be built with platform-specific modules, not as common modules
# obj-m += sirilx_gpio.o
# obj-m += sirilx_uio.o
obj-m += spi-intel.o
obj-m += spi-intel-pci.o
obj-m += spi-ciena-fpga.o
obj-m += sirilx_gnss.o
sirilx_gnss-y := gnss/sirilx_gnss.o
obj-m += ciena_raw_chardev.o
ciena_raw_chardev-y := raw_chardev/ciena_raw_chardev.o

# Note: sirilx_uio_platform.o causes kernel crash - DO NOT BUILD
# obj-m += sirilx_uio_platform.o

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(shell pwd) modules

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(shell pwd) clean

install:
	$(MAKE) -C $(KERNEL_SRC) M=$(shell pwd) modules_install

help:
	@echo "Ciena Common Platform Modules"
	@echo "=============================="
	@echo "Targets:"
	@echo "  all     - Build all common kernel modules"
	@echo "  clean   - Clean build artifacts"
	@echo "  install - Install modules to system"
	@echo ""
	@echo "Variables:"
	@echo "  KERNEL_SRC - Kernel source directory (default: /lib/modules/\$$(uname -r)/build)"

.PHONY: all clean install help

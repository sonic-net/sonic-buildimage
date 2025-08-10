PWD		= $(shell pwd)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
KVERSION   ?= $(shell uname -r)
KERNEL_SRC ?=  /lib/modules/$(KVERSION)

module_out_put_dir := $(PWD)/build
export module_out_put_dir

KERNEL_MODULES_SRC = $(PWD)/linux-5.10

PLAT_SYSFS_DIR = $(PWD)/plat_sysfs
INTEL_SPI = $(PWD)/intel_spi

export PLAT_SYSFS_DIR

platform_common-objs := platform_common_module.o dfd_tlveeprom.o
obj-m += platform_common.o
obj-m += wb_mac_bsc.o
obj-m += wb_fpga_pcie.o
obj-m += wb_pcie_dev.o
obj-m += wb_fpga_i2c_bus_drv.o
obj-m += wb_fpga_pca954x_drv.o
obj-m += wb_lpc_drv.o
obj-m += wb_i2c_dev.o
obj-m += wb_platform_i2c_dev.o
obj-m += wb_io_dev.o
obj-m += wb_eeprom_93xx46.o
obj-m += wb_spi_93xx46.o
obj-m += wb_gpio_d1500.o
obj-m += wb_gpio_device.o
obj-m += wb_i2c_ocores.o
obj-m += wb_spi_ocores.o
obj-m += wb_spi_dev.o
obj-m += wb_wdt.o
obj-m += wb_optoe.o
obj-m += wb_spi_gpio.o
obj-m += wb_spi_gpio_device.o
obj-m += wb_spi_nor_device.o
obj-m += wb_xdpe132g5c.o
obj-m += wb_uio_irq.o

all :
	$(MAKE) -C $(KERNEL_MODULES_SRC)
	$(MAKE) -C $(PLAT_SYSFS_DIR)
	$(MAKE) -C $(INTEL_SPI)
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)

clean :
	rm  -rf $(module_out_put_dir)
	rm -f ${PWD}/*.o ${PWD}/*.ko ${PWD}/*.mod.c ${PWD}/.*.cmd ${PWD}/.*.o.d ${PWD}/*.mod
	rm -f ${PWD}/Module.markers ${PWD}/Module.symvers ${PWD}/modules.order
	rm -rf ${PWD}/.tmp_versions

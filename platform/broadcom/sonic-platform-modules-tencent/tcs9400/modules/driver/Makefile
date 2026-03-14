MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
MODULES_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/modules)

EXTRA_CFLAGS+= -I$(MODULES_DIR)

obj-m += rg_pcie_dev_device.o
obj-m += rg_fpga_i2c_bus_device.o
obj-m += rg_fpga_pca954x_device.o
obj-m += rg_lpc_drv_device.o
obj-m += rg_i2c_dev_device.o
obj-m += rg_io_dev_device.o
obj-m += rg_wdt_device.o
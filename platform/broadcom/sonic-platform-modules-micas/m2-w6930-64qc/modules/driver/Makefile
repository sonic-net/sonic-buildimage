MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
MODULES_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/modules)

EXTRA_CFLAGS+= -I$(MODULES_DIR)

obj-m += wb_pcie_dev_device.o
obj-m += wb_spi_ocores_device.o
obj-m += wb_spi_dev_device.o
obj-m += wb_lpc_drv_device.o
obj-m += wb_io_dev_device.o
obj-m += wb_fpga_i2c_bus_device.o
obj-m += wb_i2c_mux_pca954x_device.o
obj-m += wb_fpga_pca954x_device.o
obj-m += wb_spi_master.o
obj-m += wb_i2c_dev_device.o
obj-m += wb_spi_dev_platform_device.o

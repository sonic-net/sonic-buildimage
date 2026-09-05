MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR = $(dir $(MAKEFILE_FILE_PATH))
MODULES_DIR = $(abspath $(MAKEFILE_DIR)/../../../common/modules)
BSP_PUBLIC_PATH = $(abspath $(MAKEFILE_DIR)/../../../public/include)

EXTRA_CFLAGS+= -I$(BSP_PUBLIC_PATH)
EXTRA_CFLAGS+= -I$(MODULES_DIR)

obj-m := wb_pcie_dev_device.o
obj-m += wb_i2c_ocores_device.o
obj-m += wb_i2c_mux_pca954x_device.o
obj-m += wb_i2c_mux_pca954x_device_b3.o
obj-m += wb_i2c_dev_device.o
obj-m += wb_io_dev_device.o

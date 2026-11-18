MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR = $(dir $(MAKEFILE_FILE_PATH))
MODULES_DIR = $(abspath $(MAKEFILE_DIR)/../../../common/modules)
FIRMWARE_UPGRADE_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/app/firmware_upgrade/firmware_driver/include)
BSP_PUBLIC_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/public/include)

EXTRA_CFLAGS+= -I$(BSP_PUBLIC_PATH)
EXTRA_CFLAGS+= -I$(MODULES_DIR)
EXTRA_CFLAGS+= -I$(FIRMWARE_UPGRADE_PATH)

obj-m := wb_pcie_dev_device.o
obj-m += wb_i2c_ocores_device.o
obj-m += wb_i2c_mux_pca954x_device.o
obj-m += wb_i2c_mux_pca954x_device_b3.o
obj-m += wb_i2c_dev_device.o
obj-m += wb_io_dev_device.o
obj-m += wb_firmware_upgrade_device.o

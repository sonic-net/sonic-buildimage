MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
MODULES_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/modules)
FIRMWARE_UPGRADE_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/app/firmware_upgrade/firmware_driver/include)

EXTRA_CFLAGS+= -I$(MODULES_DIR)
EXTRA_CFLAGS+= -I$(MODULES_DIR)/linux-5.10
EXTRA_CFLAGS+= -I$(FIRMWARE_UPGRADE_PATH)

obj-m :=  wb_pcie_dev_device.o
obj-m += wb_i2c_mux_pca954x_device.o
obj-m += wb_i2c_ocores_device.o
obj-m += wb_lpc_drv_device.o
obj-m += wb_i2c_dev_device.o
obj-m += wb_io_dev_device.o
obj-m += wb_firmware_upgrade_device.o

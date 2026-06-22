MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
MODULES_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/modules)
KERNEL_MODULES_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/modules)
FIRMWARE_UPGRADE_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/app/firmware_upgrade/firmware_driver/include)
BSP_PUBLIC_PATH = $(abspath $(MAKEFILE_FILE_PATH)/../../../../common/public/include)
COMMON_MODULE_SYMVERS = $(KERNEL_MODULES_DIR)/Module.symvers

ifneq ($(wildcard $(COMMON_MODULE_SYMVERS)),)
KBUILD_EXTRA_SYMBOLS += $(COMMON_MODULE_SYMVERS)
endif

EXTRA_CFLAGS+= -I$(BSP_PUBLIC_PATH)
EXTRA_CFLAGS+= -I$(MODULES_DIR)
EXTRA_CFLAGS+= -I$(FIRMWARE_UPGRADE_PATH)

obj-m += wb_pcie_dev_device.o
obj-m += wb_fpga_i2c_bus_device.o
obj-m += wb_i2c_mux_pca954x_device.o
obj-m += wb_fpga_pca954x_device.o
obj-m += wb_i2c_dev_device.o
#obj-m += wb_firmware_upgrade_device.o
obj-m += wb_wdt_device.o
obj-m += wb_vr_common_dev_device.o
obj-m += wb_cpld_i2c_bus_device.o
obj-m += wb_cpld_pca954x_device.o
obj-m += wb_spi_gpio_device_cpld.o
obj-m += wb_spi_dev_device.o
obj-m += wb_switch_dev_device.o
obj-m += wb_spi_gpio.o

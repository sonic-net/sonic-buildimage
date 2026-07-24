PWD		= $(shell pwd)

MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../include)
DEV_SYSFS_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../sysfs_driver/include)
SWITCH_DVR_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../switch_driver/include)
PUBLIC_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../../../public/include)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -I$(DEV_SYSFS_HEADER_DIR)
EXTRA_CFLAGS+= -I$(CURRENT_HEADER_DIR)
EXTRA_CFLAGS+= -I$(SWITCH_DVR_HEADER_DIR)
EXTRA_CFLAGS+= -I$(PUBLIC_HEADER_DIR)
EXTRA_CFLAGS+= -Wall

KBUILD_EXTRA_SYMBOLS += $(SYSFS_DRIVER_DIR)/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(SWITCH_DRIVER_DIR)/Module.symvers

obj-m := syseeprom_device_driver.o
obj-m += fan_device_driver.o
obj-m += cpld_device_driver.o
obj-m += cust_sysled_device_driver.o
obj-m += sysled_device_driver.o
obj-m += slot_device_driver.o
obj-m += psu_device_driver.o
obj-m += transceiver_device_driver.o
obj-m += temp_sensor_device_driver.o
obj-m += vol_sensor_device_driver.o
obj-m += curr_sensor_device_driver.o
obj-m += power_sensor_device_driver.o
obj-m += fpga_device_driver.o
obj-m += watchdog_device_driver.o
obj-m += system_device_driver.o
obj-m += eeprom_device_driver.o
obj-m += cabletray_device_driver.o
obj-m += misc_fw_device_driver.o
obj-m += leak_detector_device_driver.o
obj-m += sol_device_driver.o
obj-m += gcu_device_driver.o
obj-m += nvme_device_driver.o
obj-m += debug_device_driver.o
obj-m += lsw_device_driver.o
obj-m += pcie_device_driver.o
obj-m += clock_device_driver.o
obj-m += pll_device_driver.o
obj-m += avs_device_driver.o
obj-m += disk_device_driver.o
obj-m += md_device_driver.o
obj-m += dpu_device_driver.o

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(PWD) modules_install
clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order
	rm -rf $(PWD)/.tmp_versions

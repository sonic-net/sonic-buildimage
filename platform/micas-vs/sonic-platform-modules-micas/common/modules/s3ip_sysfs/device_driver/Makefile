PWD		= $(shell pwd)

MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
DEV_SYSFS_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../sysfs_driver/include)
SWITCH_DVR_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../switch_driver/include)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -I$(DEV_SYSFS_HEADER_DIR)
EXTRA_CFLAGS+= -I$(SWITCH_DVR_HEADER_DIR)
EXTRA_CFLAGS+= -Wall

KBUILD_EXTRA_SYMBOLS += $(SYSFS_DRIVER_DIR)/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(SWITCH_DRIVER_DIR)/Module.symvers

obj-m := syseeprom_device_driver.o
obj-m += fan_device_driver.o
obj-m += cpld_device_driver.o
obj-m += sysled_device_driver.o
obj-m += slot_device_driver.o
obj-m += psu_device_driver.o
obj-m += transceiver_device_driver.o
obj-m += temp_sensor_device_driver.o
obj-m += vol_sensor_device_driver.o
obj-m += curr_sensor_device_driver.o
obj-m += fpga_device_driver.o
obj-m += watchdog_device_driver.o
obj-m += system_device_driver.o
obj-m += eeprom_device_driver.o

all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)
clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd 
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order 
	rm -rf $(PWD)/.tmp_versions
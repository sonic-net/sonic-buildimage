PWD		= $(shell pwd)

MAKEFILE_FILE_PATH = $(abspath $(lastword $(MAKEFILE_LIST)))
DEV_SYSFS_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../sysfs_driver/include)
SWITCH_DVR_HEADER_DIR = $(abspath $(MAKEFILE_FILE_PATH)/../../switch_driver/include)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -I$(DEV_SYSFS_HEADER_DIR)
EXTRA_CFLAGS+= -I$(SWITCH_DVR_HEADER_DIR)
EXTRA_CFLAGS+= -Wall

s3ip_sysfs-objs := switch.o cpld_sysfs.o \
curr_sensor_sysfs.o \
fan_sysfs.o \
fpga_sysfs.o \
psu_sysfs.o \
slot_sysfs.o \
sysled_sysfs.o \
temp_sensor_sysfs.o \
transceiver_sysfs.o \
vol_sensor_sysfs.o \
watchdog_sysfs.o \
system_sysfs.o \
eeprom_sysfs.o \

obj-m := s3ip_sysfs.o

all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)
clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd 
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order 
	rm -rf $(PWD)/.tmp_versions
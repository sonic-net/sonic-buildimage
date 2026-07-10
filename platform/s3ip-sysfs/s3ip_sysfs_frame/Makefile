PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
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

obj-m := s3ip_sysfs.o

all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(sysfs_out_put_dir) ]; then mkdir -p $(sysfs_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(sysfs_out_put_dir)
clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd 
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order 
	rm -rf $(PWD)/.tmp_versions  
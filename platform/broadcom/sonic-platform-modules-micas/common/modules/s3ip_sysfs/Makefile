PWD		= $(shell pwd)
SYSFS_OUT_PUT := $(PWD)/build
sysfs_out_put_dir := $(SYSFS_OUT_PUT)/S3IP_sysfs/
sysfs_cfg_dir := $(SYSFS_OUT_PUT)/dfd_cfg/
export sysfs_out_put_dir sysfs_cfg_dir

SYSFS_DRIVER_DIR = $(PWD)/sysfs_driver
SWITCH_DRIVER_DIR = $(PWD)/switch_driver
DEVICE_DRIVER_DIR = $(PWD)/device_driver

export SYSFS_DRIVER_DIR SWITCH_DRIVER_DIR

all :
	$(MAKE) -C $(SYSFS_DRIVER_DIR)
	$(MAKE) -C $(SWITCH_DRIVER_DIR)
	$(MAKE) -C $(DEVICE_DRIVER_DIR)

clean :
	-rm  -rf $(SYSFS_OUT_PUT)
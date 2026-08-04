PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall

SUBDIR_CFG = cfg
rg_switch_driver-objs := switch_driver.o rg_module.o \
rg_fan_driver.o \
rg_eeprom_driver.o \
rg_cpld_driver.o \
rg_fpga_driver.o \
rg_led_driver.o \
rg_slot_driver.o \
rg_sensors_driver.o \
rg_psu_driver.o \
rg_sff_driver.o \
rg_watchdog_driver.o \
$(SUBDIR_CFG)/dfd_cfg.o \
$(SUBDIR_CFG)/dfd_cfg_adapter.o \
$(SUBDIR_CFG)/dfd_cfg_file.o \
$(SUBDIR_CFG)/dfd_cfg_info.o \
$(SUBDIR_CFG)/dfd_cfg_listnode.o \
$(SUBDIR_CFG)/dfd_frueeprom.o \
$(SUBDIR_CFG)/dfd_tlveeprom.o \

obj-m := rg_switch_driver.o
all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(sysfs_out_put_dir) ]; then mkdir -p $(sysfs_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(sysfs_out_put_dir)
	@if [ ! -d $(sysfs_cfg_dir) ]; then mkdir -p $(sysfs_cfg_dir) ;fi
	cp -r  $(PWD)/dfd_cfg/* $(sysfs_cfg_dir)
clean:
	rm -f $(PWD)/*.o $(PWD)/$(SUBDIR_CFG)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd $(PWD)/$(SUBDIR_CFG)/.*.cmd $(PWD)/*.mod
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order
	rm -rf $(PWD)/.tmp_versions
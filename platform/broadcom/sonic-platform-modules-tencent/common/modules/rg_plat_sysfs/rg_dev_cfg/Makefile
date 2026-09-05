PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
SUBDIR_CFG = cfg
rg_plat_dfd-objs := dfd_module.o dfd_fan_driver.o \
dfd_syseeprom_driver.o \
dfd_cpld_driver.o \
dfd_led_driver.o \
dfd_slot_driver.o \
dfd_sensors_driver.o \
dfd_psu_driver.o \
dfd_sff_driver.o \
dfd_sfpbase.o \
$(SUBDIR_CFG)/dfd_cfg.o \
$(SUBDIR_CFG)/dfd_cfg_adapter.o \
$(SUBDIR_CFG)/dfd_cfg_file.o \
$(SUBDIR_CFG)/dfd_cfg_info.o \
$(SUBDIR_CFG)/dfd_cfg_listnode.o \
$(SUBDIR_CFG)/dfd_frueeprom.o \
$(SUBDIR_CFG)/dfd_tlveeprom.o \

obj-m := rg_plat_dfd.o
all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)
clean:
	rm -f $(PWD)/*.o $(PWD)/$(SUBDIR_CFG)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd $(PWD)/$(SUBDIR_CFG)/.*.cmd $(PWD)/*.mod
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order
	rm -rf $(PWD)/.tmp_versions

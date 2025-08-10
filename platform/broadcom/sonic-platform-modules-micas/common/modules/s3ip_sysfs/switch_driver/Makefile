PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall

SUBDIR_CFG = cfg
wb_switch_driver-objs := switch_driver.o wb_module.o \
wb_fan_driver.o \
wb_eeprom_driver.o \
wb_cpld_driver.o \
wb_fpga_driver.o \
wb_led_driver.o \
wb_slot_driver.o \
wb_sensors_driver.o \
wb_psu_driver.o \
wb_sff_driver.o \
wb_watchdog_driver.o \
wb_system_driver.o \
$(SUBDIR_CFG)/dfd_cfg.o \
$(SUBDIR_CFG)/dfd_cfg_adapter.o \
$(SUBDIR_CFG)/dfd_cfg_file.o \
$(SUBDIR_CFG)/dfd_cfg_info.o \
$(SUBDIR_CFG)/dfd_cfg_listnode.o \
$(SUBDIR_CFG)/dfd_frueeprom.o \
$(SUBDIR_CFG)/dfd_tlveeprom.o \

obj-m := wb_switch_driver.o
all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)
clean:
	rm -f $(PWD)/*.o $(PWD)/$(SUBDIR_CFG)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd $(PWD)/$(SUBDIR_CFG)/.*.cmd
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order 
	rm -rf $(PWD)/.tmp_versions  
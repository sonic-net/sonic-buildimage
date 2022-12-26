PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
KBUILD_EXTRA_SYMBOLS += $(RG_PLAT_SYSFS_DIR)/rg_dev_cfg/Module.symvers

obj-m := rg_plat_switch.o
obj-m += rg_plat_fan.o
obj-m += rg_plat_psu.o
obj-m += rg_plat_sff.o
obj-m += rg_plat_sensor.o

all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)
clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd $(PWD)/*.mod
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order
	rm -rf $(PWD)/.tmp_versions

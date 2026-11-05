PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
KBUILD_EXTRA_SYMBOLS += $(PLAT_SYSFS_DIR)/dev_cfg/Module.symvers

obj-m := plat_switch.o
obj-m += plat_fan.o
obj-m += plat_psu.o
obj-m += plat_sff.o
obj-m += plat_sensor.o
obj-m += plat_slot.o

all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)
clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd $(PWD)/*.mod
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order
	rm -rf $(PWD)/.tmp_versions

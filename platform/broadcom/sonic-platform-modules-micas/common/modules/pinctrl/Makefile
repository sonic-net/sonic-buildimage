PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall

obj-m := wb_pinctrl_intel.o
obj-m += wb_gpio_c3000.o
obj-m += wb_gpio_c3000_device.o

all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)
clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd $(PWD)/*.mod
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order
	rm -rf $(PWD)/.tmp_versions

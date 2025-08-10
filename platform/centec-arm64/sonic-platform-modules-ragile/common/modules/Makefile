PWD		= $(shell pwd)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
KVERSION   ?= $(shell uname -r)
KERNEL_SRC ?=  /lib/modules/$(KVERSION)

module_out_put_dir := $(PWD)/build
export module_out_put_dir

ragile_common-objs := ragile_common_module.o dfd_tlveeprom.o
obj-m += ragile_platform.o
obj-m += i2c-mux-pca9641.o
obj-m += i2c-mux-pca954x.o
obj-m += ragile_common.o
obj-m += rtcpcf85063.o
#obj-m += tpm_tis_core.o
#obj-m += rg_spi_gpio.o
#obj-m += spi-gpio.o
#obj-m += tpm_tis_spi.o
obj-m += optoe.o
obj-m += rg_at24.o

all :
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)

clean :
	rm  -rf $(module_out_put_dir)
	rm -f ${PWD}/*.o ${PWD}/*.ko ${PWD}/*.mod.c ${PWD}/.*.cmd ${PWD}/.*.o.d
	rm -f ${PWD}/Module.markers ${PWD}/Module.symvers ${PWD}/modules.order
	rm -rf ${PWD}/.tmp_versions

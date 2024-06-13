PWD		= $(shell pwd)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
KVERSION   ?= $(shell uname -r)
KERNEL_SRC ?=  /lib/modules/$(KVERSION)

module_out_put_dir := $(PWD)/build
export module_out_put_dir

platform_common-objs := platform_common_module.o dfd_tlveeprom.o
obj-m += wb_platform.o
obj-m += platform_common.o

all :
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)

clean :
	rm  -rf $(module_out_put_dir)
	rm -f ${PWD}/*.o ${PWD}/*.ko ${PWD}/*.mod.c ${PWD}/.*.cmd ${PWD}/.*.o.d
	rm -f ${PWD}/Module.markers ${PWD}/Module.symvers ${PWD}/modules.order
	rm -rf ${PWD}/.tmp_versions

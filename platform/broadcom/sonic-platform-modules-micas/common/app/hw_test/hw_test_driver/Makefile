PWD		= $(shell pwd)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
KVERSION   ?= $(shell uname -r)
KERNEL_SRC ?=  /lib/modules/$(KVERSION)

obj-m += hw_test.o

all : 
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(common_module_dir) ]; then mkdir -p $(common_module_dir) ;fi
	cp -p  $(PWD)/*.ko $(common_module_dir)

clean :
	rm -f ${PWD}/*.o ${PWD}/*.ko ${PWD}/*.mod.c ${PWD}/.*.cmd ${PWD}/.*.o.d 
	rm -f ${PWD}/Module.markers ${PWD}/Module.symvers ${PWD}/modules.order 
	rm -rf ${PWD}/.tmp_versions  
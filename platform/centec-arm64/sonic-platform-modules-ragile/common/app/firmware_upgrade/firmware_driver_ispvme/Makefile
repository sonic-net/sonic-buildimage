PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
ifdef ENABLE_GCOV
ifeq ($(ENABLE_GCOV), y)
EXTRA_CFLAGS+= -fprofile-arcs -ftest-coverage -lgcov
endif
endif # ENABLE_GCOV
RAGILE_PRODUCT = ra_b6010_48gt4x
firmware_driver-objs := firmware_ispvme.o
firmware_driver-objs += firmware_cpld_ispvme.o firmware_cpld_upgrade_ispvme.o

#ifndef CONFIG_FRM_PRODUCT_FILE

firmware_driver-objs += $(RAGILE_PRODUCT).o
$(warning $(firmware_driver-objs))
obj-m := firmware_driver.o
all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(common_module_dir) ]; then mkdir -p $(common_module_dir) ;fi
	cp -p  $(PWD)/*.ko $(common_module_dir)
ifneq ($(KERNELRELEASE),)
obj-m:= accton_as7716_32x_cpld1.o accton_as7716_32x_fan.o  \
	    accton_as7716_32x_leds.o accton_as7716_32x_psu.o cpr_4011_4mxx.o ym2651y.o \
	    accton_i2c_cpld.o
	    
else
ifeq (,$(KERNEL_SRC))
#$(error KERNEL_SRC is not defined)
KVERSION    := $(shell uname -r)
KERNEL_DIR   = /usr/src/linux-headers-$(KVERSION)/
KERNELDIR:=$(KERNEL_DIR)
#export KERNELDIR:=${PWD}/../../../../../src/sonic-linux-kernel/linux-3.16.43/
else
KERNELDIR:=$(KERNEL_SRC)
endif
PWD:=$(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR)  M=$(PWD) modules
clean:
	rm -rf *.o *.mod.o *.mod.o *.ko .*cmd .tmp_versions Module.markers Module.symvers modules.order
endif

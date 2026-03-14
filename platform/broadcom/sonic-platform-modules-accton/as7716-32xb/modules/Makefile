ifneq ($(KERNELRELEASE),)
obj-m:= accton_as7716_32xb_cpld1.o accton_as7716_32xb_fan.o  \
	    accton_as7716_32xb_leds.o accton_as7716_32xb_psu.o \
	    accton_as7716_32xb_thermal.o accton_as7716_32xb_oom.o  accton_as7716_32xb_pmbus.o\
	    accton_as7716_32xb_sys.o accton_i2c_cpld.o
else	    
ifeq (,$(KERNEL_SRC))
$(error KERNEL_SRC is not defined)
else
KERNELDIR:=$(KERNEL_SRC)
endif
PWD:=$(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR)  M=$(PWD) modules
clean:
	rm -rf *.o *.mod.o *.mod.o *.ko .*cmd .tmp_versions Module.markers Module.symvers modules.order
endif

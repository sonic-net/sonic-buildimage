ifneq ($(KERNELRELEASE),)
obj-m:= x86-64-accton-as7926-40xfb-cpld.o x86-64-accton-as7926-40xfb-fan.o  \
        x86-64-accton-as7926-40xfb-leds.o x86-64-accton-as7926-40xfb-psu.o \
        x86-64-accton-as7926-40xfb-thermal.o x86-64-accton-as7926-40xfb-sys.o
         
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

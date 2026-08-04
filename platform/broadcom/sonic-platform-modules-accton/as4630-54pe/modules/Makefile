ifneq ($(KERNELRELEASE),)
obj-m:= x86-64-accton-as4630-54pe-cpld.o x86-64-accton-as4630-54pe-psu.o  \
	    x86-64-accton-as4630-54pe-leds.o ym2651y.o
	    
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
	rm -rf *.o *.mod.o *.mod.o *.mod.c *.ko .*cmd .tmp_versions Module.markers Module.symvers modules.order
endif

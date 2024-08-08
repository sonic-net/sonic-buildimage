ifneq ($(KERNELRELEASE),)
obj-m:= x86-64-accton-as4625-54t-cpld.o x86-64-accton-as4625-54t-fan.o  \
        x86-64-accton-as4625-54t-psu.o x86-64-accton-as4625-54t-leds.o ym2651y.o

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

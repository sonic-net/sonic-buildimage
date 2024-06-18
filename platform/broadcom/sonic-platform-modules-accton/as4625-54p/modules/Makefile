ifneq ($(KERNELRELEASE),)
obj-m:= x86-64-accton-as4625-54p-cpld.o x86-64-accton-as4625-54p-fan.o  \
        x86-64-accton-as4625-54p-psu.o x86-64-accton-as4625-54p-leds.o ym2651y.o

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

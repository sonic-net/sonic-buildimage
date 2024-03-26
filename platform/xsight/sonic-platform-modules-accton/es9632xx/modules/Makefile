ifneq ($(KERNELRELEASE),)
obj-m:= es9632-cpld.o es9632-sfp.o\
    es9632-fan.o es9632-leds.o \
    es9632-psu.o es9632-sys.o dps850.o

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

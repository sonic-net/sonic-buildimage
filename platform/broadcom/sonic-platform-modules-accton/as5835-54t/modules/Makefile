ifneq ($(KERNELRELEASE),)
obj-m:= accton_as5835_54t_cpld.o accton_as5835_54t_psu.o  \
        accton_as5835_54t_fan.o accton_as5835_54t_leds.o \
        ym2651y.o
         
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

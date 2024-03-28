ifneq ($(KERNELRELEASE),)
obj-m:= fs_n5850_48x6c_cpld.o fs_n5850_48x6c_psu.o  \
        fs_n5850_48x6c_fan.o fs_n5850_48x6c_leds.o \
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

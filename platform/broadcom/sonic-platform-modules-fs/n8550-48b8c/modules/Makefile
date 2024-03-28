ifneq ($(KERNELRELEASE),)
obj-m:= fs_i2c_cpld.o \
    fs_n8550_48b8c_fan.o fs_n8550_48b8c_leds.o \
    fs_n8550_48b8c_psu.o ym2651y.o \
    pddf_custom_psu.o

	    
CFLAGS_pddf_custom_psu.o := -I$(M)/../../../../pddf/i2c/modules/include
KBUILD_EXTRA_SYMBOLS := $(M)/../../../../pddf/i2c/Module.symvers.PDDF

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

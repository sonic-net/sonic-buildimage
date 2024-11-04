ifneq ($(KERNELRELEASE),)
obj-m:= wistron_6512_32r_cpld.o wistron_6512_32r_fan.o \
		wistron_6512_32r_oom.o wistron_6512_32r_psu.o \
		wistron_6512_32r_thermal.o i2c-imc.o oldjc42.o \
		wistron_6512_32r_leds.o wistron_6512_32r_syseeprom.o
else
ifeq (,$(KERNEL_SRC))
#$(error KERNEL_SRC is not defined)
KVERSION=3.16.0-8-amd64
KERNEL_DIR   = /usr/src/linux-headers-$(KVERSION)/
KERNELDIR:=$(KERNEL_DIR)
else
KERNELDIR:=$(KERNEL_SRC)
endif
PWD:=$(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR)  M=$(PWD) modules
clean:
	rm -rf  *.o *.mod.o *.mod.o *.ko .*cmd *.mod.c *.mod .tmp_versions Module.markers Module.symvers modules.order
endif

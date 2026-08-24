obj-m:=x86-64-accton-as7816-64x-fan.o x86-64-accton-as7816-64x-leds.o \
       x86-64-accton-as7816-64x-psu.o accton_i2c_cpld.o ym2651y.o pddf_custom_fan.o

CFLAGS_pddf_custom_fan.o := -I$(M)/../../../../pddf/i2c/modules/include
KBUILD_EXTRA_SYMBOLS := $(KERNEL_SRC)/Module.symvers.PDDF


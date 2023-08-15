
MODULE_NAME  = x86-64-ufispace-s7801-54xs-cpld.o x86-64-ufispace-s7801-54xs-sys-eeprom.o x86-64-ufispace-s7801-54xs-lpc.o pddf_custom_sysstatus_module.o
obj-m       := $(MODULE_NAME)

CFLAGS_pddf_custom_sysstatus_module.o := -I$(M)/../../../../pddf/i2c/modules/include
KBUILD_EXTRA_SYMBOLS := $(M)/../../../../pddf/i2c/Module.symvers.PDDF

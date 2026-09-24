
MODULE_NAME  = x86-64-ufispace-s9301-32d-cpld.o x86-64-ufispace-s9301-32d-sys-eeprom.o x86-64-ufispace-s9301-32d-lpc.o pddf_custom_sysstatus_module.o x86-64-ufispace-s9301-32d-i2c-smbus.o
obj-m       := $(MODULE_NAME)

CFLAGS_pddf_custom_sysstatus_module.o := -I$(M)/../../../../pddf/i2c/modules/include
KBUILD_EXTRA_SYMBOLS := $(KERNEL_SRC)/Module.symvers.PDDF

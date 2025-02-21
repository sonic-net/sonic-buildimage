
x86-64-ufispace-s9321-64eo-cpld-objs := x86-64-ufispace-s9321-64eo-cpld-main.o x86-64-ufispace-s9321-64eo-cpld-mux.o
MODULE_NAME  = x86-64-ufispace-s9321-64eo-cpld.o x86-64-ufispace-s9321-64eo-sys-eeprom.o x86-64-ufispace-s9321-64eo-lpc.o pddf_custom_sysstatus_module.o pddf_custom_cpldmux_driver.o
obj-m       := $(MODULE_NAME)

CFLAGS_pddf_custom_sysstatus_module.o := -I$(M)/../../../../pddf/i2c/modules/include
CFLAGS_pddf_custom_cpldmux_driver.o := -I$(M)/../../../../pddf/i2c/modules/include
KBUILD_EXTRA_SYMBOLS := $(M)/../../../../pddf/i2c/Module.symvers.PDDF

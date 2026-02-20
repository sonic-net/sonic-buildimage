
x86-64-ufispace-s9311-64d-cpld-objs := x86-64-ufispace-s9311-64d-cpld-main.o x86-64-ufispace-s9311-64d-cpld-mux.o
MODULE_NAME  = x86-64-ufispace-s9311-64d-cpld.o x86-64-ufispace-s9311-64d-sys-eeprom.o x86-64-ufispace-s9311-64d-lpc.o pddf_custom_sysstatus_module.o 
obj-m       := $(MODULE_NAME)

CFLAGS_pddf_custom_sysstatus_module.o := -I$(M)/../../../../pddf/i2c/modules/include
KBUILD_EXTRA_SYMBOLS := $(KERNEL_SRC)/Module.symvers.PDDF

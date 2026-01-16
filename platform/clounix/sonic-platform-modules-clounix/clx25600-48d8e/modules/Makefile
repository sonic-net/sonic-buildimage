obj-m:= pddf_custom_psu.o
obj-m += pddf_custom_optoe.o
obj-m += pddf_custom_wdt.o
obj-m += clounix_fpga_register_init.o
obj-m += clounix_fpga_sysfs.o
CFLAGS_pddf_custom_psu.o := -I$(M)/../../../../pddf/i2c/modules/include
KBUILD_EXTRA_SYMBOLS := $(M)/../../../../pddf/i2c/Module.symvers.PDDF

ccflags-y += -I/sonic/platform/pddf/i2c/modules/include
EXTRA_CFLAGS += -g

CPLDMUX_TARGET = pddf_custom_cpldmux_driver_module
$(CPLDMUX_TARGET)-objs := cpldmux_driver/pddf_cpldmux_driver.o

XCVR_TARGET = pddf_custom_xcvr_driver_module
$(XCVR_TARGET)-objs := xcvr_driver/pddf_xcvr_api.o xcvr_driver/pddf_xcvr_driver.o

obj-m += $(CPLDMUX_TARGET).o
obj-m += $(XCVR_TARGET).o
obj-m += baseboard_cpld.o come_cpld.o pddf_custom_fpga_algo.o pddf_custom_wdt.o i2c-mux-pca954x.o pddf_custom_fpga_extend.o

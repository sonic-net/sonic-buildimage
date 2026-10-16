ccflags-y += -I/sonic/platform/pddf/i2c/modules/include
ccflags-y += -I$(src)/psu_driver
EXTRA_CFLAGS += -g

CPLDMUX_TARGET = pddf_custom_cpldmux_driver_module
$(CPLDMUX_TARGET)-objs := cpldmux_driver/pddf_cpldmux_driver.o

XCVR_TARGET = pddf_custom_xcvr_driver_module
$(XCVR_TARGET)-objs := xcvr_driver/pddf_xcvr_api.o xcvr_driver/pddf_xcvr_driver.o

PSU_TARGET = pddf_custom_psu_driver_module
$(PSU_TARGET)-objs := psu_driver/pddf_psu_api.o psu_driver/pddf_psu_driver.o

obj-m += $(CPLDMUX_TARGET).o
obj-m += $(XCVR_TARGET).o
obj-m += $(PSU_TARGET).o
obj-m += baseboard_cpld.o come_cpld.o pddf_custom_fpga_algo.o pddf_custom_wdt.o i2c-mux-pca954x.o pddf_custom_fpga_extend.o

# backported standard drivers from mainline
obj-m += tmp117.o mp2975.o mp9941.o tps25990.o tps546b24.o

# add icp20100 driver
obj-m += icp201xx_core.o icp201xx_i2c.o icp201xx.o

# add icx_dimmtemp driver
obj-m += icx_dimmtemp.o
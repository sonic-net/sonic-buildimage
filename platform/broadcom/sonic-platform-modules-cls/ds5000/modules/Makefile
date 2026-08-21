TARGET:= pddf_custom_psu_driver_module
$(TARGET)-objs := ./psu_driver/pddf_psu_api.o ./psu_driver/pddf_psu_driver.o

obj-m := $(TARGET).o

# isl68137 already in upstream and configured.

obj-m += icp201xx_core.o icp201xx_i2c.o icp201xx.o i2c-imc.o i2c-mux-pca954x.o baseboard_cpld.o i2c-xiic-fpga.o switchboard_fpga.o pddf_custom_wdt.o pddf_custom_xcvr.o pddf_custom_mux.o pddf_xcvr_patch.o

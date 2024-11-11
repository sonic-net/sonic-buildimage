TARGET1:= pddf_custom_psu_driver_module
$(TARGET1)-objs := ./psu_driver/pddf_psu_api.o ./psu_driver/pddf_psu_driver.o

TARGET2:= icp201xx
$(TARGET2)-objs := ./icp201xx/icp201xx_core.o ./icp201xx/icp201xx_i2c.o ./icp201xx/icp201xx.o

obj-m := $(TARGET1).o $(TARGET2).o
obj-m += pddf_custom_fpga_algo.o pddf_custom_fpga_extend.o i2c-imc.o isl68137.o i2c-mux-pca954x.o baseboard_cpld.o pddf_custom_wdt.o pddf_custom_xcvr.o pddf_custom_mux.o pddf_custom_xcvr_api.o
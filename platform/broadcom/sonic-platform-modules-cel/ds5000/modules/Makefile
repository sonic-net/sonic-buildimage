TARGET1:= icp201xx
$(TARGET1)-objs := ./icp201xx/icp201xx_core.o ./icp201xx/icp201xx_i2c.o ./icp201xx/icp201xx.o

obj-m := $(TARGET1).o
obj-m += pddf_custom_fpga_algo.o pddf_custom_fpga_extend.o i2c-imc.o i2c-mux-pca954x.o baseboard_cpld.o pddf_custom_wdt.o pddf_custom_xcvr.o pddf_custom_mux.o pddf_custom_psu_api.o pddf_custom_xcvr_api.o
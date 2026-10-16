
TARGET:= pddf_custom_led_module
$(TARGET)-objs := ./led_driver/pddf_custom_led_module.o

TARGET1:= pddf_custom_psu_driver_module
$(TARGET1)-objs := ./psu_driver/pddf_psu_api.o ./psu_driver/pddf_psu_driver.o

TARGET2:= pddf_custom_xcvr_driver_module
$(TARGET2)-objs := ./xcvr_driver/pddf_xcvr_api.o ./xcvr_driver/pddf_xcvr_driver.o

obj-m := $(TARGET).o $(TARGET1).o $(TARGET2).o
obj-m += ds5001_fpgapci_driver.o ds5001_fpgapci_module.o pddf_custom_fpga_algo.o pddf_custom_fpga_extend.o pddf_custom_wdt.o custom_lpc_basecpld.o ds5001_dimm_temp.o ds5001_mp2975.o

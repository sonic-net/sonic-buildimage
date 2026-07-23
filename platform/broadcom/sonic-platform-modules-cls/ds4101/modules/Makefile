
TARGET:= pddf_custom_led_module
$(TARGET)-objs := ./led_driver/pddf_custom_led_module.o

TARGET1:= pddf_custom_psu_driver_module
$(TARGET1)-objs := ./psu_driver/pddf_psu_api.o ./psu_driver/pddf_psu_driver.o

obj-m := $(TARGET).o $(TARGET1).o
obj-m += pddf_custom_fpga_algo.o pddf_custom_fpga_extend.o pddf_custom_wdt.o custom_lpc_basecpld.o mp2880.o mp5023.o i2c-imc.o

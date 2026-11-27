TARGET:= pddf_custom_psu_driver_module
$(TARGET)-objs := ./psu_driver/pddf_psu_api.o ./psu_driver/pddf_psu_driver.o

obj-m := $(TARGET).o
obj-m += pddf_custom_wdt.o lpc_baseboard_cpld1.o lpc_baseboard_cpld2.o optoe.o

PSU_TARGET:= pddf_custom_psu_driver_module
$(PSU_TARGET)-objs := ./psu_driver/pddf_psu_api.o ./psu_driver/pddf_psu_driver.o

LED_TARGET:= pddf_custom_led_module
$(LED_TARGET)-objs := ./led_driver/pddf_custom_led_module.o

obj-m := $(PSU_TARGET).o $(LED_TARGET).o
obj-m += i2c-imc.o pddf_custom_wdt.o mc24lc64t.o baseboard_cpld.o pddf_custom_fpga_algo.o pddf_custom_fpga_extend.o mp5023.o tps536c7.o ti-ads7142.o

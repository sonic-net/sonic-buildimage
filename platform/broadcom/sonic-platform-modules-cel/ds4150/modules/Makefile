LED_TARGET:= pddf_custom_led_module
$(LED_TARGET)-objs := ./led_driver/pddf_custom_led_module.o

obj-m := $(LED_TARGET).o
obj-m += i2c-imc.o pddf_custom_wdt.o baseboard_cpld.o pddf_custom_fpga_algo.o pddf_custom_fpga_extend.o

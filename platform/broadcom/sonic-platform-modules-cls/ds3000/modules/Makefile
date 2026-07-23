LED_TARGET:= pddf_custom_led_module
$(LED_TARGET)-objs := ./led_driver/pddf_custom_led_module.o

PSU_TARGET:= pddf_custom_psu_driver_module
$(PSU_TARGET)-objs := ./psu_driver/pddf_psu_api.o ./psu_driver/pddf_psu_driver.o

obj-m := mc24lc64t.o baseboard_cpld.o pddf_custom_fpga_algo.o pddf_custom_fpga_extend.o
obj-m += $(LED_TARGET).o $(PSU_TARGET).o

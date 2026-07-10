TARGET:= pddf_custom_psu_driver_module
$(TARGET)-objs := ./psu_driver/pddf_psu_api.o ./psu_driver/pddf_psu_driver.o

obj-m := $(TARGET).o
obj-m += mc24lc64t.o pddf_custom_fpga_algo.o pddf_custom_fpga_extend.o lpc_basecpld.o

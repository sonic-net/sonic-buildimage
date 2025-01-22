PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
#ifdef ENABLE_GCOV
#ifeq ($(ENABLE_GCOV), y)
#EXTRA_CFLAGS+= -fprofile-arcs -ftest-coverage -lgcov
#endif
#endif # ENABLE_GCOV

obj-m := wb_lm75.o
obj-m += wb_tmp401.o
obj-m += wb_i2c_mux_pca9641.o
obj-m += wb_i2c_mux_pca954x.o
obj-m += wb_i2c_i801.o
obj-m += wb_i2c_algo_bit.o
obj-m += wb_i2c_gpio.o
obj-m += wb_i2c_gpio_device.o
obj-m += wb_at24.o
obj-m += wb_pmbus_core.o
obj-m += wb_csu550.o
obj-m += wb_ina3221.o
obj-m += wb_isl68137.o
obj-m += wb_tps53622.o
obj-m += wb_ucd9000.o

all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)
clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd $(PWD)/*.mod
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order
	rm -rf $(PWD)/.tmp_versions

PWD = $(shell pwd)

obj-m += wb_i2c_designware_core.o
wb_i2c_designware_core-y := i2c-designware-common.o
wb_i2c_designware_core-y += i2c-designware-master.o
wb_i2c_designware_core-$(CONFIG_I2C_DESIGNWARE_SLAVE) += i2c-designware-slave.o
obj-m += wb_i2c_designware_platform.o
wb_i2c_designware_platform-y := i2c-designware-platdrv.o
wb_i2c_designware_platform-$(CONFIG_I2C_DESIGNWARE_AMDPSP) += i2c-designware-amdpsp.o
wb_i2c_designware_platform-$(CONFIG_I2C_DESIGNWARE_BAYTRAIL) += i2c-designware-baytrail.o
obj-$(CONFIG_I2C_DESIGNWARE_PCI) += wb_i2c_designware_pci.o
wb_i2c_designware_pci-y := i2c-designware-pcidrv.o

all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	cp -p  $(PWD)/*.ko $(module_out_put_dir)
clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd $(PWD)/*.order $(PWD)/*.symvers $(PWD)/*.mod

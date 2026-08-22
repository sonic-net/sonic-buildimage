ifneq ($(KERNELRELEASE),)

WB_KERNEL_MAJOR := $(VERSION)
WB_KERNEL_MINOR := $(PATCHLEVEL)
WB_KERNEL_PATCH := $(SUBLEVEL)
WB_KERNEL_SERIES := k6

ifeq ($(filter 5,$(WB_KERNEL_MAJOR)),5)
WB_KERNEL_SERIES := k5
endif

ccflags-y += -DWB_KERNEL_MAJOR=$(WB_KERNEL_MAJOR)
ccflags-y += -DWB_KERNEL_MINOR=$(WB_KERNEL_MINOR)
ccflags-y += -DWB_KERNEL_PATCH=$(WB_KERNEL_PATCH)
ccflags-y += -I$(M)/../public/include

# Template for split implementations:
# wb_example-objs := wb_example_core.o wb_example_$(WB_KERNEL_SERIES).o

obj-m   += s3ip_sysfs/
platform_common-objs := platform_common/platform_common_module.o platform_common/dfd_tlveeprom.o
obj-m   += platform_common.o
obj-m	+= wb_logic_dev_common.o
obj-m	+= wb_platform_i2c_dev.o
obj-m	+= wb_i2c_dev.o
obj-m	+= wb_i2c_mux_pca954x.o
obj-m	+= wb_fpga_i2c_bus_drv.o
obj-m   += wb_fpga_seu_drv.o
obj-m   += wb_cpld_i2c_bus_drv.o
obj-m	+= wb_fpga_pca954x_drv.o
obj-m   += wb_cpld_pca954x_drv.o
obj-m	+= wb_spi_93xx46.o
obj-m	+= wb_wdt.o
obj-m	+= hw_test.o
obj-m	+= wb_spi_nor_device.o
obj-m	+= platform_common.o
obj-m	+= wb_csu550.o
obj-m	+= wb_spi_dev.o
obj-m	+= wb_indirect_dev.o
obj-m	+= wb_xdpe132g5c.o
obj-m	+= wb_spi_gpio_device.o
obj-m 	+= wb_dev_resume.o
obj-m   += platform_common.o
obj-m 	+= wb_logic_mux.o
obj-m   += wb_rtc_inst85053.o
obj-m   += wb_spi_master.o
obj-m	+= wb_test_exception.o
obj-m   += wb_mdio_dev.o
obj-m   += wb_switch_dev.o
obj-m   += wb_ast2700_mac1_rgmii_fix.o
obj-m   += ct7148.o
obj-m   += wb_dummy_dev.o
obj-m 	+= wb_common_logic_mux.o
obj-m 	+= wb_lmk03328.o
obj-$(CONFIG_WB_PLATFORM_IRQ) 	+= wb_platform_irq.o

wb_gpio_init_drv-objs := wb_gpio_init_drv/wb_gpio_init.o

obj-m   += wb_gpio_init_drv.o
ifeq ($(WB_KERNEL_SERIES),k5)
obj-m   += wb_peci_tool.o
endif

else

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) clean

endif

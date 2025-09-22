obj-m := pddf_custom_fpga_algo.o
obj-m += pddf_custom_mdio_algo.o
obj-m += nh_tda38740.o
obj-m += nh_isl68137.o
obj-m += nh_pmbus_core.o
obj-m += nh_tmp464.o
obj-m += nh_adm1266.o

CFLAGS_pddf_custom_fpga_algo.o := -I$(M)/../../../../pddf/i2c/modules/include
KBUILD_EXTRA_SYMBOLS := $(M)/../../../../pddf/i2c/Module.symvers.PDDF

ifeq ($(KERNELRELEASE),)
    ifeq (,$(KERNEL_SRC))
        $(error KERNEL_SRC is not defined)
    endif
    KERNELDIR := $(KERNEL_SRC)
    PWD := $(shell pwd)

default:
    $(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
    rm -rf *.o *.mod.o *.mod.c *.ko .*cmd .tmp_versions Module.markers Module.symvers modules.order
endif

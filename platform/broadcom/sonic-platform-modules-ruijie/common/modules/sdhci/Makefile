PWD		= $(shell pwd)

EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall

sdhci_pci-objs := sdhci-pci-core.o sdhci-pci-o2micro.o sdhci-pci-arasan.o \
				   sdhci-pci-dwc-mshc.o sdhci-pci-gli.o
obj-m += sdhci_pci.o


all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(PWD) modules
	@if [ ! -d $(module_out_put_dir) ]; then mkdir -p $(module_out_put_dir) ;fi
	@if [ -f $(PWD)/*.ko ]; then cp -p $(PWD)/*.ko $(module_out_put_dir);fi
clean:
	rm -f $(PWD)/*.o $(PWD)/*.ko $(PWD)/*.mod.c $(PWD)/.*.cmd 
	rm -f $(PWD)/Module.markers $(PWD)/Module.symvers $(PWD)/modules.order 
	rm -rf $(PWD)/.tmp_versions  
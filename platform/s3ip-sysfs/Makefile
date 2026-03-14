PWD		= $(shell pwd)
SYSFS_OUT_PUT := $(PWD)/build
sysfs_out_put_dir := $(SYSFS_OUT_PUT)/S3IP_sysfs/
export sysfs_out_put_dir
KERNEL_SRC=/lib/modules/$(shell uname -r)
export KERNEL_SRC

SYSFS_DRIVER_DIR = $(PWD)/s3ip_sysfs_frame
SWITCH_DRIVER_DIR = $(PWD)/demo_driver

KBUILD_EXTRA_SYMBOLS += $(SYSFS_DRIVER_DIR)/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(SWITCH_DRIVER_DIR)/Module.symvers
export KBUILD_EXTRA_SYMBOLS

all :
	$(MAKE) -C $(SYSFS_DRIVER_DIR)
	$(MAKE) -C $(SWITCH_DRIVER_DIR)

clean :
	-rm  -rf $(SYSFS_OUT_PUT)
	$(MAKE) -C $(SYSFS_DRIVER_DIR) clean
	$(MAKE) -C $(SWITCH_DRIVER_DIR) clean



install: 
	install -d $(DESTDIR)/lib/modules/s3ip/
	install -D $(sysfs_out_put_dir)/*.ko \
		$(DESTDIR)/lib/modules/s3ip/
	install -D scripts/s3ip_load.py \
		$(DESTDIR)/$(prefix)/bin/s3ip_load.py
	install -D scripts/s3ip_sysfs_conf.json \
		$(DESTDIR)/etc/s3ip/s3ip_sysfs_conf.json
	install -D scripts/s3ip_sysfs_tool.sh \
		$(DESTDIR)/$(prefix)/bin/s3ip_sysfs_tool.sh
	install -D scripts/s3ip-sysfs.service \
		$(DESTDIR)/etc/systemd/system/s3ip-sysfs.service


uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/s3ip_load.py
	-rm -f $(DESTDIR)/lib/modules/s3ip/
	-rm -f $(DESTDIR)/etc/s3ip
	-rm -f $(DESTDIR)/$(prefix)/bin/s3ip_sysfs_tool.sh
	-rm -f $(DESTDIR)/etc/systemd/system/s3ip-sysfs.service


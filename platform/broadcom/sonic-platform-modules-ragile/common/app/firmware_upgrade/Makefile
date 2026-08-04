top_srcdir:=$(shell pwd)
include $(top_srcdir)/Rules.mk

firmware-y:=
firmware-y += firmware_driver
firmware-y += firmware_upgrade

.PHONY: all
all: build

.PHONY: build
build: $(firmware-y)
$(foreach dir,$(firmware-y),$(eval $(call compile_dirs,$(dir))))

.PHONY: rpmpkg
rpmpkg:
ifeq ("$(CONFIG_CPLD_UPGRADE_ISPVME)", "y")
	#$(RPMPKG) $(install_cpld_dir) firmware-cpld-ispvme.spec git
endif

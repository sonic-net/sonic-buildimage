# monit package

MONIT_VERSION = 5.34.3-1

export MONIT_VERSION

MONIT = monit_$(MONIT_VERSION)_$(CONFIGURED_ARCH).deb
$(MONIT)_SRC_PATH = $(SRC_PATH)/monit
SONIC_MAKE_DEBS += $(MONIT)

MONIT_DBG = monit-dbgsym_$(MONIT_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(MONIT),$(MONIT_DBG)))

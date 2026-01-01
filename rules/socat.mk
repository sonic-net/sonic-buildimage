# socat packages

SOCAT_VERSION = 1.7.4.1-3

export SOCAT_VERSION

SOCAT = socat_$(SOCAT_VERSION)_$(CONFIGURED_ARCH).deb
$(SOCAT)_SRC_PATH = $(SRC_PATH)/socat
SONIC_MAKE_DEBS += $(SOCAT)

SOCAT_DBG = socat-dbgsym_$(SOCAT_VERSION)_$(CONFIGURED_ARCH).deb
$(SOCAT_DBG)_DEPENDS += $(SOCAT)
$(SOCAT_DBG)_RDEPENDS += $(SOCAT)
$(eval $(call add_derived_package,$(SOCAT),$(SOCAT_DBG)))

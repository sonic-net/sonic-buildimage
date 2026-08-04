# iccpd package

ICCPD_VERSION = 0.0.5

ICCPD = iccpd_$(ICCPD_VERSION)_$(CONFIGURED_ARCH).deb
$(ICCPD)_DEPENDS += $(LIBNL_GENL3_DEV) $(LIBNL_CLI_DEV)
$(ICCPD)_RDEPENDS += $(LIBNL_GENL3) $(LIBNL_CLI)
$(ICCPD)_SRC_PATH = $(SRC_PATH)/iccpd
SONIC_DPKG_DEBS += $(ICCPD)

ICCPD_DBG = iccpd-dbg_$(ICCPD_VERSION)_$(CONFIGURED_ARCH).deb
$(ICCPD_DBG)_DEPENDS += $(ICCPD)
$(ICCPD_DBG)_RDEPENDS += $(ICCPD)
$(eval $(call add_derived_package,$(ICCPD),$(ICCPD_DBG)))

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += iccpd

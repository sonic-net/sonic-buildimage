# ipmitool packages
IPMITOOL_VERSION = 1.8.19
IPMITOOL_VERSION_SUFFIX = 4+deb12u1
IPMITOOL_VERSION_FULL = $(IPMITOOL_VERSION)-$(IPMITOOL_VERSION_SUFFIX)
IPMITOOL = ipmitool_$(IPMITOOL_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(IPMITOOL)_SRC_PATH = $(SRC_PATH)/ipmitool
SONIC_MAKE_DEBS += $(IPMITOOL)
IPMITOOL_DBG = ipmitool-dbgsym_$(IPMITOOL_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(IPMITOOL),$(IPMITOOL_DBG)))
DBG_SRC_ARCHIVE += ipmitool 
# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
# Export these variables so they can be used in a sub-make
export IPMITOOL_VERSION
export IPMITOOL_VERSION_FULL 
export IPMITOOL
export IPMITOOL_DBG

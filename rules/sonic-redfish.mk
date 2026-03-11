# sonic-redfish packages
# This builds two Debian packages:
# 1. bmcweb
# 2. sonic-dbus-bridge


# Package versions
SONIC_REDFISH_VERSION = 1.0.0
SONIC_REDFISH_SUBVERSION = 1

# Build sonic-redfish packages only in Trixie environment
# bmcweb requires gcc-13 and modern C++20 features
ifeq ($(BLDENV), trixie)

BMCWEB = bmcweb_$(SONIC_REDFISH_VERSION)_$(CONFIGURED_ARCH).deb
$(BMCWEB)_SRC_PATH = $(SRC_PATH)/sonic-redfish
SONIC_MAKE_DEBS += $(BMCWEB)

# bmcweb debug package
BMCWEB_DBG = bmcweb-dbg_$(SONIC_REDFISH_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(BMCWEB),$(BMCWEB_DBG)))

# sonic-dbus-bridge package - built with dpkg-buildpackage from sonic-dbus-bridge/ directory
SONIC_DBUS_BRIDGE = sonic-dbus-bridge_$(SONIC_REDFISH_VERSION)_$(CONFIGURED_ARCH).deb
$(SONIC_DBUS_BRIDGE)_SRC_PATH = $(SRC_PATH)/sonic-redfish/sonic-dbus-bridge
SONIC_DPKG_DEBS += $(SONIC_DBUS_BRIDGE)

# sonic-dbus-bridge debug symbols package
SONIC_DBUS_BRIDGE_DBGSYM = sonic-dbus-bridge-dbgsym_$(SONIC_REDFISH_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SONIC_DBUS_BRIDGE),$(SONIC_DBUS_BRIDGE_DBGSYM)))

endif

# Export variables for the build
export SONIC_REDFISH_VERSION SONIC_REDFISH_SUBVERSION

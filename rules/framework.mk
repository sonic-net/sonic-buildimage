# framework package

FRAMEWORK = framework_1.0.0_$(CONFIGURED_ARCH).deb
$(FRAMEWORK)_SRC_PATH = $(SRC_PATH)/sonic-framework
$(FRAMEWORK)_DEPENDS += $(LIBSWSSCOMMON_DEV)

$(FRAMEWORK)_RDEPENDS += $(LIBSWSSCOMMON)
SONIC_DPKG_DEBS += $(FRAMEWORK)

FRAMEWORK_DBG = framework-dbg_1.0.0_$(CONFIGURED_ARCH).deb
$(FRAMEWORK_DBG)_DEPENDS += $(FRAMEWORK)
$(FRAMEWORK_DBG)_RDEPENDS += $(FRAMEWORK)
$(eval $(call add_derived_package,$(FRAMEWORK),$(FRAMEWORK_DBG)))

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += sonic-framework

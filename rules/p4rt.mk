# SONiC P4RT package

SONIC_P4RT_VERSION = 0.0.1

SONIC_P4RT = sonic-p4rt_$(SONIC_P4RT_VERSION)_$(CONFIGURED_ARCH).deb
$(SONIC_P4RT)_SRC_PATH = $(SRC_PATH)/sonic-p4rt
$(SONIC_P4RT)_DEPENDS += $(LIBSWSSCOMMON_DEV)
$(SONIC_P4RT)_RDEPENDS += $(LIBSWSSCOMMON)
SONIC_MAKE_DEBS += $(SONIC_P4RT)

SONIC_P4RT_DBG = sonic-p4rt-dbgsym_$(SONIC_P4RT_VERSION)_$(CONFIGURED_ARCH).deb
$(SONIC_P4RT_DBG)_DEPENDS += $(SONIC_P4RT)
$(SONIC_P4RT_DBG)_RDEPENDS += $(SONIC_P4RT)
$(eval $(call add_derived_package,$(SONIC_P4RT),$(SONIC_P4RT_DBG)))

export SONIC_P4RT SONIC_P4RT_DBG SONIC_P4RT_VERSION

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
DBG_SRC_ARCHIVE += sonic-p4rt

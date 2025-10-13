# SONiC host userd binary package

SONIC_HOST_USERD = sonic-host-userd_1.0-1_$(CONFIGURED_ARCH).deb
$(SONIC_HOST_USERD)_SRC_PATH = $(SRC_PATH)/sonic-host-services/userd
$(SONIC_HOST_USERD)_DEPENDS += $(LIBSWSSCOMMON_DEV)
$(SONIC_HOST_USERD)_RDEPENDS += $(SONIC_HOST_SERVICES_DATA) $(LIBSWSSCOMMON)
SONIC_DPKG_DEBS += $(SONIC_HOST_USERD)

# Debug symbols package
SONIC_HOST_USERD_DBG = sonic-host-userd-dbgsym_1.0-1_$(CONFIGURED_ARCH).deb
$(SONIC_HOST_USERD_DBG)_DEPENDS += $(SONIC_HOST_USERD)
$(SONIC_HOST_USERD_DBG)_RDEPENDS += $(SONIC_HOST_USERD)
$(eval $(call add_derived_package,$(SONIC_HOST_USERD),$(SONIC_HOST_USERD_DBG)))

# Add to debug source archive for debugging
DBG_SRC_ARCHIVE += sonic-host-services/userd

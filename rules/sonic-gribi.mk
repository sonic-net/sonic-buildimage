# sonic-gribi package

SONIC_GRIBI_VERSION = 1.0.0
SONIC_GRIBI = sonic-gribi_$(SONIC_GRIBI_VERSION)_$(CONFIGURED_ARCH).deb
$(SONIC_GRIBI)_SRC_PATH = $(SRC_PATH)/sonic-gribi
SONIC_DPKG_DEBS += $(SONIC_GRIBI)

SONIC_GRIBI_DBG = sonic-gribi-dbgsym_$(SONIC_GRIBI_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SONIC_GRIBI),$(SONIC_GRIBI_DBG)))

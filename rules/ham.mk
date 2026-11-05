HAM_VERSION := 1.0.0

# Main package
HAM = ham_$(HAM_VERSION)_$(CONFIGURED_ARCH).deb
$(HAM)_SRC_PATH = $(SRC_PATH)/ham
$(HAM)_DEPENDS  = $(LIBSWSSCOMMON_DEV)
$(HAM)_RDEPENDS = $(LIBSWSSCOMMON)

SONIC_DPKG_DEBS += $(HAM)

# Additional packages
LIBHAM_DEV = libham-dev_$(HAM_VERSION)_$(CONFIGURED_ARCH).deb
LIBHAM     = libham_$(HAM_VERSION)_$(CONFIGURED_ARCH).deb
LIBNSS_HAM = libnss-ham_$(HAM_VERSION)_$(CONFIGURED_ARCH).deb
LIBNSS_SAC = libnss-sac_$(HAM_VERSION)_$(CONFIGURED_ARCH).deb

$(eval $(call add_derived_package,$(HAM),$(LIBHAM_DEV)))
$(eval $(call add_derived_package,$(HAM),$(LIBHAM)))
$(eval $(call add_derived_package,$(HAM),$(LIBNSS_HAM)))
$(eval $(call add_derived_package,$(HAM),$(LIBNSS_SAC)))


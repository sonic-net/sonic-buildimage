# Debian Trixie hiredis with atomic close-on-exec sockets.
ifeq ($(BLDENV),trixie)
LIBHIREDIS_VERSION = 1.2.0-6+sonic1
LIBHIREDIS = libhiredis1.1.0_$(LIBHIREDIS_VERSION)_$(CONFIGURED_ARCH).deb
$(LIBHIREDIS)_SRC_PATH = $(SRC_PATH)/hiredis
SONIC_MAKE_DEBS += $(LIBHIREDIS)

LIBHIREDIS_DEV = libhiredis-dev_$(LIBHIREDIS_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBHIREDIS),$(LIBHIREDIS_DEV)))
LIBHIREDIS_DBG = libhiredis1.1.0-dbgsym_$(LIBHIREDIS_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBHIREDIS),$(LIBHIREDIS_DBG)))
endif

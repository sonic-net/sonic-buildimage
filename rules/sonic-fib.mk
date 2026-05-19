# libFIB package

LIBFIB_VERSION = 1.0.0
LIBFIB_NAME = libnexthopgroup

LIBFIB = $(LIBFIB_NAME)_$(LIBFIB_VERSION)_$(CONFIGURED_ARCH).deb
$(LIBFIB)_SRC_PATH = $(SRC_PATH)/libraries/sonic-fib
$(LIBFIB)_VERSION = $(LIBFIB_VERSION)
$(LIBFIB)_NAME = $(LIBFIB_NAME)

SONIC_DPKG_DEBS += $(LIBFIB)

LIBFIB_DEV = $(LIBFIB_NAME)-dev_$(LIBFIB_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBFIB),$(LIBFIB_DEV)))





# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += libraries/sonic-fib


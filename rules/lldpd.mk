# lldpd package

LLDPD_VERSION = 1.0.16
LLDPD_VERSION_SUFFIX = 1+deb12u1
LLDPD_VERSION_FULL = $(LLDPD_VERSION)-$(LLDPD_VERSION_SUFFIX)

LLDPD = lldpd_$(LLDPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(LLDPD)_DEPENDS += $(LIBSNMP_DEV)
$(LLDPD)_RDEPENDS += $(LIBSNMP)
$(LLDPD)_SRC_PATH = $(SRC_PATH)/lldpd
SONIC_MAKE_DEBS += $(LLDPD)

LIBLLDPCTL = liblldpctl-dev_$(LLDPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LLDPD),$(LIBLLDPCTL)))

LLDPD_DBG = lldpd-dbgsym_$(LLDPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LLDPD),$(LLDPD_DBG)))

# Export these variables so they can be used in a sub-make
export LLDPD_VERSION
export LLDPD_VERSION_FULL
export LLDPD
export LIBLLDPCTL
export LLDPD_DBG

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += lldpd

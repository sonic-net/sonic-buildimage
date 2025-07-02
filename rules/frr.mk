# FRRouting (frr) package

FRR_VERSION = 10.3
FRR_SUBVERSION = 0
FRR_BRANCH = frr-10.3
FRR_TAG = frr-10.3
export FRR_VERSION FRR_SUBVERSION FRR_BRANCH FRR_TAG


FRR = frr_$(FRR_VERSION)-sonic-$(FRR_SUBVERSION)_$(CONFIGURED_ARCH).deb
$(FRR)_DEPENDS += $(LIBSNMP_DEV) $(LIBYANG3_DEV)
$(FRR)_RDEPENDS += $(LIBYANG3)
$(FRR)_SRC_PATH = $(SRC_PATH)/sonic-frr
SONIC_MAKE_DEBS += $(FRR)

FRR_PYTHONTOOLS = frr-pythontools_$(FRR_VERSION)-sonic-$(FRR_SUBVERSION)_all.deb
$(eval $(call add_extra_package,$(FRR),$(FRR_PYTHONTOOLS)))

FRR_DBG = frr-dbgsym_$(FRR_VERSION)-sonic-$(FRR_SUBVERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_extra_package,$(FRR),$(FRR_DBG)))

FRR_SNMP = frr-snmp_$(FRR_VERSION)-sonic-$(FRR_SUBVERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_extra_package,$(FRR),$(FRR_SNMP)))

FRR_SNMP_DBG = frr-snmp-dbgsym_$(FRR_VERSION)-sonic-$(FRR_SUBVERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_extra_package,$(FRR),$(FRR_SNMP_DBG)))

export FRR FRR_PYTHONTOOLS FRR_DBG FRR_SNMP FRR_SNMP_DBG

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += sonic-frr

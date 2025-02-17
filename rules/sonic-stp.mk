# STP package
#
STP_VERSION = 1.0.0
export STP_VERSION

STP = stp_$(STP_VERSION)_$(CONFIGURED_ARCH).deb
$(STP)_SRC_PATH = $(SRC_PATH)/sonic-stp
$(STP)_DEPENDS += $(LIBSWSSCOMMON_DEV)
$(STP)_RDEPENDS += $(LIBSWSSCOMMON)
SONIC_DPKG_DEBS += $(STP)

STP_DBG = stp-dbg_$(STP_VERSION)_$(CONFIGURED_ARCH).deb
$(STP_DBG)_DEPENDS += $(STP)
$(STP_DBG)_RDEPENDS += $(STP)
$(eval $(call add_derived_package,$(STP),$(STP_DBG)))

export STP

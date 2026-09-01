# SONiC TAM package

TAM_VERSION = 1.0.0
export TAM_VERSION

SONIC_TAM = tam_$(TAM_VERSION)_amd64.deb
$(SONIC_TAM)_SRC_PATH = $(SRC_PATH)/sonic-tam
$(SONIC_TAM)_DEPENDS = $(LIBSWSSCOMMON_DEV)
$(SONIC_TAM)_RDEPENDS = $(LIBSWSSCOMMON)
SONIC_DPKG_DEBS += $(SONIC_TAM)

TAM_DBG = tam-dbg_1.0.0_amd64.deb
$(TAM_DBG)_DEPENDS += $(TAM)
$(TAM_DBG)_RDEPENDS += $(TAM)
$(eval $(call add_derived_package,$(TAM),$(TAM_DBG)))


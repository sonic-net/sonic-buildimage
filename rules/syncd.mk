# only used for non-vs platforms

ifneq ($(CONFIGURED_PLATFORM),vs)
ifneq ($(CONFIGURED_PLATFORM),aspeed)
ifneq ($(CONFIGURED_PLATFORM),nokia-vs)

ifeq ($(ENABLE_ASAN), y)
SYNCD = syncd-asan_1.0.0_$(CONFIGURED_ARCH).deb
else
SYNCD = syncd_1.0.0_$(CONFIGURED_ARCH).deb
endif
$(SYNCD)_RDEPENDS += $(LIBSAIREDIS) $(LIBSAIMETADATA)
$(SYNCD)_DEB_BUILD_PROFILES += syncd
ifeq ($(ENABLE_ASAN), y)
$(SYNCD)_DEB_BUILD_PROFILES += asan
endif
$(SYNCD)_SRC_PATH = $(SRC_PATH)/sonic-sairedis
$(SYNCD)_DEPENDS += $(LIBSWSSCOMMON_DEV) $(LIBSAIREDIS)
$(SYNCD)_RDEPENDS += $(LIBSWSSCOMMON)
$(SYNCD)_DEB_BUILD_OPTIONS = nocheck
SONIC_DPKG_DEBS += $(SYNCD)

ifeq ($(ENABLE_SYNCD_RPC),y)
SYNCD_RPC = syncd-rpc_1.0.0_$(CONFIGURED_ARCH).deb
$(SYNCD_RPC)_RDEPENDS += $(LIBSAIREDIS) $(LIBSAIMETADATA)
$(eval $(call add_derived_package,$(SYNCD),$(SYNCD_RPC)))

# Inject libthrift build dependency for RPC build
$(SYNCD)_DEPENDS += $(LIBSWSSCOMMON_DEV) $(LIBTHRIFT_DEV)
$(SYNCD)_DEB_BUILD_PROFILES += rpc
endif

ifeq ($(ENABLE_ASAN), y)
SYNCD_DBG = syncd-asan-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
else
SYNCD_DBG = syncd-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
endif
$(SYNCD_DBG)_DEPENDS += $(SYNCD)
$(SYNCD_DBG)_RDEPENDS += $(SYNCD)
$(eval $(call add_derived_package,$(SYNCD),$(SYNCD_DBG)))

ifeq ($(ENABLE_SYNCD_RPC),y)
SYNCD_RPC_DBG = syncd-rpc-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SYNCD_RPC_DBG)_DEPENDS += $(SYNCD_RPC)
$(SYNCD_RPC_DBG)_RDEPENDS += $(SYNCD_RPC)
$(eval $(call add_derived_package,$(SYNCD),$(SYNCD_RPC_DBG)))
endif

ifeq ($(ENABLE_PY2_MODULES), n)
    $(SYNCD)_DEB_BUILD_PROFILES += nopython2
endif

endif
endif
endif

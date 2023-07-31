# only used for non-vs platforms

ifneq ($(CONFIGURED_PLATFORM),vs)

SYNCD = syncd_1.0.0_$(CONFIGURED_ARCH).deb
$(SYNCD)_RDEPENDS += $(LIBSAIREDIS) $(LIBSAIMETADATA)
$(SYNCD)_DEB_BUILD_PROFILES += syncd
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

SYNCD_DBGSYM = syncd-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SYNCD_DBGSYM)_DEPENDS += $(SYNCD)
$(SYNCD_DBGSYM)_RDEPENDS += $(SYNCD)
$(eval $(call add_derived_package,$(SYNCD),$(SYNCD_DBGSYM)))

ifeq ($(ENABLE_SYNCD_RPC),y)
SYNCD_RPC_DBGSYM = syncd-rpc-dbgsym_1.0.0_$(CONFIGURED_ARCH).deb
$(SYNCD_RPC_DBGSYM)_DEPENDS += $(SYNCD_RPC)
$(SYNCD_RPC_DBGSYM)_RDEPENDS += $(SYNCD_RPC)
$(eval $(call add_derived_package,$(SYNCD),$(SYNCD_RPC_DBGSYM)))
endif

ifeq ($(ENABLE_PY2_MODULES), n)
    $(SYNCD)_DEB_BUILD_PROFILES += nopython2
endif

endif

# swss package

ifeq ($(ENABLE_ASAN), y)
SWSS = swss-asan_1.0.0_$(CONFIGURED_ARCH).deb
else
SWSS = swss_1.0.0_$(CONFIGURED_ARCH).deb
endif
$(SWSS)_SRC_PATH = $(SRC_PATH)/sonic-swss
$(SWSS)_DEPENDS += $(LIBSAIREDIS_DEV) $(LIBSAIMETADATA_DEV) $(LIBTEAM_DEV) \
    $(LIBTEAMDCTL) $(LIBTEAM_UTILS) $(LIBSWSSCOMMON_DEV)  $(LIBFIB_DEV) \
    $(LIBSAIVS) $(LIBSAIVS_DEV) $(STP)\
    $(PROTOBUF) $(PROTOBUF_LITE) $(PROTOBUF_DEV) $(LIB_SONIC_DASH_API)
$(SWSS)_UNINSTALLS = $(LIBSAIVS_DEV)
ifeq ($(ENABLE_ASAN), y)
$(SWSS)_DEB_BUILD_PROFILES += asan
endif

$(SWSS)_RDEPENDS += $(LIBSAIREDIS) $(LIBSAIMETADATA) $(LIBTEAM) \
    $(LIBTEAMDCTL) $(LIBSWSSCOMMON) $(PYTHON3_SWSSCOMMON) $(LIBFIB) \
    $(PROTOBUF) $(PROTOBUF_LITE) $(PYTHON3_PROTOBUF) $(LIB_SONIC_DASH_API)
SONIC_DPKG_DEBS += $(SWSS)

ifeq ($(ENABLE_ASAN), y)
SWSS_DBG = swss-dbg-asan_1.0.0_$(CONFIGURED_ARCH).deb
else
SWSS_DBG = swss-dbg_1.0.0_$(CONFIGURED_ARCH).deb
endif
$(SWSS_DBG)_DEPENDS += $(SWSS)
$(SWSS_DBG)_RDEPENDS += $(SWSS)
$(eval $(call add_derived_package,$(SWSS),$(SWSS_DBG)))

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += sonic-swss


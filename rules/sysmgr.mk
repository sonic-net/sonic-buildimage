# sysmgr package

SYSMGR = sysmgr_1.0.0_$(CONFIGURED_ARCH).deb
$(SYSMGR)_SRC_PATH = $(SRC_PATH)/sonic-sysmgr
$(SYSMGR)_DEPENDS += $(LIBSWSSCOMMON_DEV) \
 			$(PROTOBUF) $(PROTOBUF_LITE) $(PROTOBUF_DEV) $(PROTOBUF_COMPILER)

$(SYSMGR)_RDEPENDS += $(LIBSWSSCOMMON) $(PROTOBUF)
SONIC_DPKG_DEBS += $(SYSMGR)

SYSMGR_DBG = sysmgr-dbg_1.0.0_$(CONFIGURED_ARCH).deb
$(SYSMGR_DBG)_DEPENDS += $(SYSMGR)
$(SYSMGR_DBG)_RDEPENDS += $(SYSMGR)
$(eval $(call add_derived_package,$(SYSMGR),$(SYSMGR_DBG)))

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += sonic-sysmgr

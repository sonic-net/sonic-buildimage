# Lucius packet handler for Alpine

PKT_HANDLER = lucius-pkthandler_1.0-2_$(CONFIGURED_ARCH).deb
$(PKT_HANDLER)_SRC_PATH = $(SRC_PATH)/sonic-alpine/services/pkt-handler
$(PKT_HANDLER)_DEPENDS += $(LIBNL3_DEV) $(LIBNL_GENL3_DEV)
$(PKT_HANDLER)_RDEPENDS += $(LIBNL3) $(LIBNL_GENL3)
SONIC_DPKG_DEBS += $(PKT_HANDLER)

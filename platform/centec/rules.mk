include $(PLATFORM_PATH)/platform-modules-centec-e582.mk
include $(PLATFORM_PATH)/platform-modules-embedway.mk
include $(PLATFORM_PATH)/platform-modules-centec-v682.mk
include $(PLATFORM_PATH)/sdk.mk
include $(PLATFORM_PATH)/docker-syncd-centec.mk
include $(PLATFORM_PATH)/docker-syncd-centec-rpc.mk
include $(PLATFORM_PATH)/docker-saiserver-centec.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk

SONIC_ALL += $(SONIC_ONE_IMAGE)

# Inject centec sai into syncd
$(SYNCD)_DEPENDS += $(CENTEC_SAI) $(CENTEC_SAI_DEV)
$(SYNCD)_UNINSTALLS += $(CENTEC_SAI_DEV) $(CENTEC_SAI)

ifeq ($(ENABLE_SYNCD_RPC),y)
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV)
endif

# Runtime dependency on centec sai is set only for syncd
$(SYNCD)_RDEPENDS += $(CENTEC_SAI)

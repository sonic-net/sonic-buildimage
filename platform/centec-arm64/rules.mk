include $(PLATFORM_PATH)/sai.mk
include $(PLATFORM_PATH)/docker-syncd-centec.mk
include $(PLATFORM_PATH)/docker-syncd-centec-rpc.mk
include $(PLATFORM_PATH)/docker-saiserver-centec.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk
include $(PLATFORM_PATH)/tsingma-bsp.mk
include $(PLATFORM_PATH)/platform-modules-centec-e530.mk

SONIC_ALL += $(SONIC_ONE_IMAGE) \
             $(DOCKER_FPM)
#             $(DOCKER_SYNCD_CENTEC_RPC)

# Inject centec sai into syncd
$(SYNCD)_DEPENDS += $(CENTEC_SAI) $(CENTEC_SAI_DEV)
$(SYNCD)_UNINSTALLS += $(CENTEC_SAI_DEV) $(CENTEC_SAI)

ifeq ($(ENABLE_SYNCD_RPC),y)
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV)
endif

# Runtime dependency on centec sai is set only for syncd
$(SYNCD)_RDEPENDS += $(CENTEC_SAI)

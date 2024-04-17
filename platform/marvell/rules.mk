include $(PLATFORM_PATH)/sai.mk
include $(PLATFORM_PATH)/docker-syncd-mrvl.mk
include $(PLATFORM_PATH)/docker-syncd-mrvl-rpc.mk
include $(PLATFORM_PATH)/docker-saiserver-mrvl.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/platform-marvell.mk
ifeq ($(CONFIGURED_ARCH),$(filter $(CONFIGURED_ARCH),arm64 armhf))
include $(PLATFORM_PATH)/mrvl-prestera.mk
include $(PLATFORM_PATH)/platform-nokia.mk
endif

SONIC_ALL += $(SONIC_ONE_IMAGE) \
             $(DOCKER_FPM) 	\
             $(DOCKER_SYNCD_MRVL_RPC)

# Inject mrvl sai into syncd
$(SYNCD)_DEPENDS += $(MRVL_SAI)
$(SYNCD)_UNINSTALLS += $(MRVL_SAI)

ifeq ($(ENABLE_SYNCD_RPC),y)
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV)
endif

# Runtime dependency on mrvl sai is set only for syncd
$(SYNCD)_RDEPENDS += $(MRVL_SAI)

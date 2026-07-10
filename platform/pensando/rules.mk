include $(PLATFORM_PATH)/docker-dpu-base.mk
include $(PLATFORM_PATH)/docker-dpu.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/sdk.mk
include $(PLATFORM_PATH)/docker-syncd-pensando.mk
include $(PLATFORM_PATH)/dsc-drivers.mk
include $(PLATFORM_PATH)/sonic-platform-modules-dpu.mk

SONIC_ALL += $(SONIC_ONE_IMAGE) \
             $(DOCKER_FPM)

# Inject pensando sai into syncd
$(SYNCD)_DEPENDS += $(PENSANDO_SAI)
$(SYNCD)_UNINSTALLS += $(PENSANDO_SAI)

#Runtime dependency on pensando sai is set only for syncd
$(SYNCD)_RDEPENDS += $(PENSANDO_SAI)

include $(PLATFORM_PATH)/syncd-vs.mk
include $(PLATFORM_PATH)/sonic-version.mk
include $(PLATFORM_PATH)/docker-syncd-vs.mk
include $(PLATFORM_PATH)/lemmingsai.mk
include $(PLATFORM_PATH)/pkt-handler.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/onie.mk
include $(PLATFORM_PATH)/kvm-image.mk
include $(PLATFORM_PATH)/raw-image.mk

# Disable platform
#include $(PLATFORM_PATH)/platform-alpinevs.mk

# Inject lemming sai into syncd
$(SYNCD)_DEPENDS += $(LEMMINGSAI)
$(SYNCD)_UNINSTALLS += $(LEMMINGSAI)

SONIC_ALL += $(SONIC_ONE_IMAGE) $(SONIC_KVM_IMAGE) $(SYNCD_VS) $(SONIC_RAW_IMAGE)

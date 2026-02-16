# Aspeed platform rules
include $(PLATFORM_PATH)/platform-modules-aspeed.mk
include $(PLATFORM_PATH)/one-image.mk

SONIC_ALL += $(SONIC_ONE_IMAGE)

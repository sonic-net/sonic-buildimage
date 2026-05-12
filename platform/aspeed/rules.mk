include $(PLATFORM_PATH)/platform-modules-ast-evb.mk
include $(PLATFORM_PATH)/platform-modules-nexthop.mk
include $(PLATFORM_PATH)/aspeed-platform-services.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/recipes/installer-tftp.mk

SONIC_ALL += $(SONIC_ONE_IMAGE)

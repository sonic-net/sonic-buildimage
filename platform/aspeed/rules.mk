include $(PLATFORM_PATH)/platform-modules-ast-evb.mk
include $(PLATFORM_PATH)/platform-modules-nexthop.mk
<<<<<<< HEAD
include $(PLATFORM_PATH)/platform-modules-nvidia-bmc.mk
=======
include $(PLATFORM_PATH)/../nexthop-common/sonic-platform-nexthop-utils-wheel.mk
>>>>>>> cae8a44e0 (NOS-8349: Move shared broadcom/aspeed files to new nexthop-common directory (#5205))
include $(PLATFORM_PATH)/aspeed-platform-services.mk
include $(PLATFORM_PATH)/nvidia-hw-mgmt.mk
include $(PLATFORM_PATH)/platform-modules-arista.mk
include $(PLATFORM_PATH)/platform-modules-nokia.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/recipes/installer-tftp.mk

SONIC_ALL += $(SONIC_ONE_IMAGE)

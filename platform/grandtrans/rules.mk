include $(PLATFORM_PATH)/sai.mk
include $(PLATFORM_PATH)/grandtrans-modules.mk
include $(PLATFORM_PATH)/docker-syncd-grandtrans.mk
include $(PLATFORM_PATH)/docker-syncd-grandtrans-rpc.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/docker-ptf-grandtrans.mk

SONIC_ALL += $(SONIC_ONE_IMAGE) $(DOCKER_FPM)

# Inject grandtrans sai into syncd
$(SYNCD)_DEPENDS += $(GRANDTRANS_SAI) $(GRANDTRANS_SAI_DEV)
$(SYNCD)_UNINSTALLS += $(GRANDTRANS_SAI_DEV)

# Force the target bootloader for grandtrans platforms to grub regardless of arch
override TARGET_BOOTLOADER = grub

# Runtime dependency on grandtrans sai is set only for syncd
$(SYNCD)_RDEPENDS += $(GRANDTRANS_SAI)

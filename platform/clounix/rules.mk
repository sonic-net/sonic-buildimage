include $(PLATFORM_PATH)/sai.mk
include $(PLATFORM_PATH)/clounix-modules.mk
include $(PLATFORM_PATH)/docker-syncd-clounix.mk
include $(PLATFORM_PATH)/docker-syncd-clounix-rpc.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk
include $(PLATFORM_PATH)/docker-ptf-clounix.mk
include $(PLATFORM_PATH)/docker-saiserver-clounix.mk

DSSERVE = dsserve
$(DSSERVE)_URL = "$(BUILD_PUBLIC_URL)/20190307/dsserve"
SONIC_ONLINE_FILES += $(DSSERVE)

SONIC_ALL += $(SONIC_ONE_IMAGE) $(DOCKER_FPM)

# Inject clounix sai into syncd
$(SYNCD)_DEPENDS += $(CLOUNIX_SAI) $(CLOUNIX_SAI_DEV)
$(SYNCD)_UNINSTALLS += $(CLOUNIX_SAI_DEV)

ifeq ($(ENABLE_SYNCD_RPC),y)
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV)
endif

# Runtime dependency on clounix sai is set only for syncd
$(SYNCD)_RDEPENDS += $(CLOUNIX_SAI)

include $(PLATFORM_PATH)/sai.mk
include $(PLATFORM_PATH)/clx-utils.mk
include $(PLATFORM_PATH)/clounix-modules.mk
include $(PLATFORM_PATH)/platform-modules-pegatron.mk
include $(PLATFORM_PATH)/docker-syncd-clounix.mk
include $(PLATFORM_PATH)/docker-syncd-clounix-rpc.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk
include $(PLATFORM_PATH)/docker-ptf-clounix.mk
include $(PLATFORM_PATH)/docker-saiserver-clounix.mk

DSSERVE = dsserve
$(DSSERVE)_URL = "https://sonicstorage.blob.core.windows.net/packages/20190307/dsserve?sv=2015-04-05&sr=b&sig=lk7BH3DtW%2F5ehc0Rkqfga%2BUCABI0UzQmDamBsZH9K6w%3D&se=2038-05-06T22%3A34%3A45Z&sp=r"
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

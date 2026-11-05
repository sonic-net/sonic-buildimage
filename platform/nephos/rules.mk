include $(PLATFORM_PATH)/sai.mk
include $(PLATFORM_PATH)/nephos-modules.mk
include $(PLATFORM_PATH)/platform-modules-ingrasys.mk
include $(PLATFORM_PATH)/platform-modules-accton.mk
include $(PLATFORM_PATH)/platform-modules-cig.mk
include $(PLATFORM_PATH)/docker-syncd-nephos.mk
include $(PLATFORM_PATH)/docker-syncd-nephos-rpc.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk

NPX_DIAG = npx_diag
$(NPX_DIAG)_URL = "https://github.com/NephosInc/SONiC/raw/master/sdk/npx_diag"

WARM_VERIFIER = warm-verifier
$(WARM_VERIFIER)_URL = "https://github.com/NephosInc/SONiC/raw/master/sai/warm-verifier"

DSSERVE = dsserve
$(DSSERVE)_URL = "$(BUILD_PUBLIC_URL)/20190307/dsserve"

SONIC_ONLINE_FILES += $(NPX_DIAG) $(WARM_VERIFIER) $(DSSERVE)

SONIC_ALL += $(SONIC_ONE_IMAGE) $(DOCKER_FPM)

# Inject nephos sai into syncd
$(SYNCD)_DEPENDS += $(NEPHOS_SAI) $(NEPHOS_SAI_DEV)
$(SYNCD)_UNINSTALLS += $(NEPHOS_SAI_DEV)

ifeq ($(ENABLE_SYNCD_RPC),y)
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV)
endif

# Runtime dependency on nephos sai is set only for syncd
$(SYNCD)_RDEPENDS += $(NEPHOS_SAI)

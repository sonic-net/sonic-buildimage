include $(PLATFORM_PATH)/sai-modules.mk
include $(PLATFORM_PATH)/sai.mk
# TODO: re-enable once the following have been updated for bookworm:
# * Nokia
# * Juniper
# * Ragile
# * Ufispace
#include $(PLATFORM_PATH)/platform-modules-nokia.mk
include $(PLATFORM_PATH)/platform-modules-dell.mk
include $(PLATFORM_PATH)/platform-modules-arista.mk
#include $(PLATFORM_PATH)/platform-modules-ingrasys.mk
include $(PLATFORM_PATH)/platform-modules-accton.mk
include $(PLATFORM_PATH)/platform-modules-alphanetworks.mk
#include $(PLATFORM_PATH)/platform-modules-inventec.mk
include $(PLATFORM_PATH)/platform-modules-cel.mk
#include $(PLATFORM_PATH)/platform-modules-delta.mk
include $(PLATFORM_PATH)/platform-modules-quanta.mk
##include $(PLATFORM_PATH)/platform-modules-mitac.mk
#include $(PLATFORM_PATH)/platform-modules-juniper.mk
#include $(PLATFORM_PATH)/platform-modules-brcm-xlr-gts.mk
#include $(PLATFORM_PATH)/platform-modules-ruijie.mk
#include $(PLATFORM_PATH)/platform-modules-ragile.mk
#include $(PLATFORM_PATH)/platform-modules-tencent.mk
#include $(PLATFORM_PATH)/platform-modules-ufispace.mk
include $(PLATFORM_PATH)/docker-syncd-brcm.mk
include $(PLATFORM_PATH)/docker-syncd-brcm-rpc.mk
include $(PLATFORM_PATH)/docker-saiserver-brcm.mk
ifeq ($(INCLUDE_PDE), y)
include $(PLATFORM_PATH)/docker-pde.mk
include $(PLATFORM_PATH)/sonic-pde-tests.mk
endif
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/raw-image.mk
include $(PLATFORM_PATH)/one-aboot.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk
include $(PLATFORM_PATH)/docker-syncd-brcm-dnx.mk
include $(PLATFORM_PATH)/docker-syncd-brcm-dnx-rpc.mk
ifeq ($(INCLUDE_GBSYNCD), y)
include $(PLATFORM_PATH)/../components/docker-gbsyncd-credo.mk
include $(PLATFORM_PATH)/../components/docker-gbsyncd-broncos.mk
endif

BCMCMD = bcmcmd
$(BCMCMD)_URL = "https://sonicstorage.blob.core.windows.net/packages/20190307/bcmcmd?sv=2015-04-05&sr=b&sig=sUdbU7oVbh5exbXXHVL5TDFBTWDDBASHeJ8Cp0B0TIc%3D&se=2038-05-06T22%3A34%3A19Z&sp=r"

DSSERVE = dsserve
$(DSSERVE)_URL = "https://sonicstorage.blob.core.windows.net/packages/20190307/dsserve?sv=2015-04-05&sr=b&sig=lk7BH3DtW%2F5ehc0Rkqfga%2BUCABI0UzQmDamBsZH9K6w%3D&se=2038-05-06T22%3A34%3A45Z&sp=r"

SONIC_ONLINE_FILES += $(BCMCMD) $(DSSERVE)

SONIC_ALL += $(SONIC_ONE_IMAGE) $(SONIC_ONE_ABOOT_IMAGE) \
             $(DOCKER_FPM)

# Inject brcm sai into syncd
$(SYNCD)_DEPENDS += $(BRCM_XGS_SAI) $(BRCM_XGS_SAI_DEV)
$(SYNCD)_UNINSTALLS += $(BRCM_XGS_SAI_DEV) $(BRCM_XGS_SAI)

ifeq ($(ENABLE_SYNCD_RPC),y)
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV)
endif

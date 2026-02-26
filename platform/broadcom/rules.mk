include $(PLATFORM_PATH)/sai-modules.mk
include $(PLATFORM_PATH)/sai-xgs.mk
include $(PLATFORM_PATH)/sai-dnx.mk
include $(PLATFORM_PATH)/sswsyncd.mk
# TODO(trixie): re-enable modules after updating for Trixie
include $(PLATFORM_PATH)/platform-modules-nokia.mk
include $(PLATFORM_PATH)/platform-modules-dell.mk
include $(PLATFORM_PATH)/platform-modules-arista.mk
include $(PLATFORM_PATH)/platform-modules-nexthop.mk
#include $(PLATFORM_PATH)/platform-modules-ingrasys.mk
#include $(PLATFORM_PATH)/platform-modules-accton.mk
#include $(PLATFORM_PATH)/platform-modules-alphanetworks.mk
#include $(PLATFORM_PATH)/platform-modules-inventec.mk
include $(PLATFORM_PATH)/platform-modules-cel.mk
#include $(PLATFORM_PATH)/platform-modules-delta.mk
#include $(PLATFORM_PATH)/platform-modules-quanta.mk
##include $(PLATFORM_PATH)/platform-modules-mitac.mk
#include $(PLATFORM_PATH)/platform-modules-juniper.mk
#include $(PLATFORM_PATH)/platform-modules-brcm-xlr-gts.mk
#include $(PLATFORM_PATH)/platform-modules-ruijie.mk
#include $(PLATFORM_PATH)/platform-modules-ragile.mk
include $(PLATFORM_PATH)/platform-modules-supermicro.mk
#include $(PLATFORM_PATH)/platform-modules-tencent.mk
include $(PLATFORM_PATH)/platform-modules-ufispace.mk
#include $(PLATFORM_PATH)/platform-modules-micas.mk
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
include $(PLATFORM_PATH)/../components/docker-gbsyncd-agera2.mk
include $(PLATFORM_PATH)/../components/docker-gbsyncd-milleniob.mk
endif

BCMCMD = bcmcmd
$(BCMCMD)_URL = "$(BUILD_PUBLIC_URL)/20190307/bcmcmd"

DSSERVE = dsserve
$(DSSERVE)_URL = "$(BUILD_PUBLIC_URL)/20190307/dsserve"

SONIC_ONLINE_FILES += $(BCMCMD) $(DSSERVE)

SONIC_ALL += $(SONIC_ONE_IMAGE) $(SONIC_ONE_ABOOT_IMAGE) \
             $(DOCKER_FPM)

# Inject brcm sai into syncd
$(SYNCD)_DEPENDS += $(BRCM_XGS_SAI) $(BRCM_XGS_SAI_DEV)
$(SYNCD)_UNINSTALLS += $(BRCM_XGS_SAI_DEV) $(BRCM_XGS_SAI)

ifeq ($(ENABLE_SYNCD_RPC),y)
# Remove the libthrift_0.11.0 dependency injected by rules/syncd.mk
$(SYNCD)_DEPENDS := $(filter-out $(LIBTHRIFT_DEV),$($(SYNCD)_DEPENDS))
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV)
endif

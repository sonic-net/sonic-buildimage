# docker image for brcm syncd

DOCKER_SYNCD_PLATFORM_CODE = brcm
include $(PLATFORM_PATH)/../template/docker-syncd-bookworm.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD)
$(DOCKER_SYNCD_BASE)_DEPENDS += $(BRCM_XGS_SAI)
$(DOCKER_SYNCD_BASE)_DEPENDS += $(SSWSYNCD)
$(DOCKER_SYNCD_BASE)_FILES += $(RDB-CLI)

$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(SYNCD_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG)

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd
$(DOCKER_SYNCD_BASE)_MACHINE = broadcom

$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /host/warmboot:/var/warmboot
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /usr/share/sonic/device/x86_64-broadcom_common:/usr/share/sonic/device/x86_64-broadcom_common:ro

$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += bcmcmd:/usr/bin/bcmcmd
$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += bcmsh:/usr/bin/bcmsh
$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += bcm_common:/usr/bin/bcm_common

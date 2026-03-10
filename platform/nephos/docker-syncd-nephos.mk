# docker image for nephos syncd

DOCKER_SYNCD_PLATFORM_CODE = nephos
include $(PLATFORM_PATH)/../template/docker-syncd-base.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD)
$(DOCKER_SYNCD_BASE)_FILES += $(DSSERVE) $(NPX_DIAG)

$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(SYNCD_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG)

SONIC_STRETCH_DOCKERS += $(DOCKER_SYNCD_BASE)
SONIC_STRETCH_DBG_DOCKERS += $(DOCKER_SYNCD_BASE_DBG)

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd

$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /var/run/docker-syncd:/var/run/sswsyncd

$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += npx_diag:/usr/bin/npx_diag

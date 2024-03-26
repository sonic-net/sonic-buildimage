# docker image for XSight Labs syncd

DOCKER_SYNCD_PLATFORM_CODE = xsight
include $(PLATFORM_PATH)/../template/docker-syncd-bullseye.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD) $(LIBTHRIFT)
ifeq ($(XBM_EXISTS),y)
$(DOCKER_SYNCD_BASE)_DEPENDS += $(XSIGHT_XBM)
endif

$(DOCKER_SYNCD_BASE)_RUN_OPT += --env ASAN_SAI_PLUGIN=${ASAN_SAI_PLUGIN} # used in address-sanitizer sai-plugin check

$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(SYNCD_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG)

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd

$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /host/warmboot:/var/warmboot


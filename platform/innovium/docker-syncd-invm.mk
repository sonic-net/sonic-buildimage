# docker image for innovium syncd

DOCKER_SYNCD_PLATFORM_CODE = invm
include $(PLATFORM_PATH)/../template/docker-syncd-base.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD) $(PYTHON_SDK_API) $(INVM_LIBSAI) $(INVM_SHELL)

$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(SYNCD_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG)

SONIC_BUSTER_DOCKERS += $(DOCKER_SYNCD_BASE)
SONIC_BUSTER_DBG_DOCKERS += $(DOCKER_SYNCD_BASE_DBG)

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd
$(DOCKER_SYNCD_BASE)_MACHINE = innovium

$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /host/warmboot:/var/warmboot

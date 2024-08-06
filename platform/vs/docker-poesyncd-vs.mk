# docker image for vs poesyncd

DOCKER_POESYNCD_PLATFORM_CODE = vs
include $(PLATFORM_PATH)/../template/docker-poesyncd-base.mk

$(DOCKER_POESYNCD_BASE)_DEPENDS += $(SYNCD_VS)

$(DOCKER_POESYNCD_BASE)_DBG_DEPENDS += $(SYNCD_VS_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG) \
                                $(LIBSAIVS_DBG)

$(DOCKER_POESYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_POESYNCD_BASE)_PACKAGE_NAME = poesyncd

$(DOCKER_POESYNCD_BASE)_RUN_OPT += -v /host/warmboot:/var/warmboot

# docker image for vs gbsyncd

DOCKER_GBSYNCD_PLATFORM_CODE = vs
include $(PLATFORM_PATH)/../template/docker-gbsyncd-bookworm.mk

$(DOCKER_GBSYNCD_BASE)_DEPENDS += $(SYNCD_VS) \
                              $(LIBNL3_DEV) \
                              $(LIBNL3)

$(DOCKER_GBSYNCD_BASE)_DBG_DEPENDS += $(SYNCD_VS_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG) \
                                $(LIBSAIVS_DBG)

$(DOCKER_GBSYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_GBSYNCD_BASE)_PACKAGE_NAME = gbsyncd

$(DOCKER_GBSYNCD_BASE)_RUN_OPT += -v /host/warmboot:/var/warmboot

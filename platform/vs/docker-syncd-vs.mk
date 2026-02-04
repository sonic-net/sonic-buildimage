# docker image for vs syncd

DOCKER_SYNCD_PLATFORM_CODE = vs
include $(PLATFORM_PATH)/../template/docker-syncd-bookworm.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD_VS) \
                              $(LIBNL_ROUTE3_DEV) \
                              $(LIBNL3_DEV) \
                              $(LIBNL3)

ifeq ($(INCLUDE_FIPS), y)
$(DOCKER_SYNCD_BASE)_DEPENDS += $(FIPS_OPENSSH_CLIENT)
else
$(DOCKER_SYNCD_BASE)_DEPENDS += $(OPENSSH_CLIENT)
endif

$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(SYNCD_VS_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG) \
                                $(LIBSAIVS_DBG)

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd

$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /host/warmboot:/var/warmboot

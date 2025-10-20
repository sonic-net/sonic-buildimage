# docker image for clounix syncd

DOCKER_SYNCD_PLATFORM_CODE = clounix
include $(PLATFORM_PATH)/../template/docker-syncd-base.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD) $(CLOUNIX_SAI_DEV) $(CLOUNIX_WARM_VERIFIER)
$(DOCKER_SYNCD_BASE)_FILES =  $(DSSERVE)

$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(SYNCD_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG)
                                
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /host/warmboot:/var/warmboot
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /var/run/docker-syncd:/var/run/sswsyncd

$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += clx_diag:/usr/bin/clx_diag
$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += clx_ipython:/usr/bin/clx_ipython
$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += clx_icling:/usr/bin/clx_icling

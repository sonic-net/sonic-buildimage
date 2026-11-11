# docker image for grandtrans syncd

DOCKER_SYNCD_PLATFORM_CODE = grandtrans
include $(PLATFORM_PATH)/../template/docker-syncd-bookworm.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD) $(GRANDTRANS_SAI) $(GRANDTRANS_WARM_VERIFIER) $(LIBCAREPLUS)
ifeq ($(INCLUDE_XEUS_CLING),yes)
$(DOCKER_SYNCD_BASE)_DEPENDS += $(XEUS_CLING)
endif
$(DOCKER_SYNCD_BASE)_FILES =  $(DSSERVE)

$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(SYNCD_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG)

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd
$(DOCKER_SYNCD_BASE)_MACHINE = grandtrans

$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /host/warmboot:/var/warmboot
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /var/run/docker-syncd:/var/run/sswsyncd

$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += gt_diag:/usr/bin/gt_diag
$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += gt_ipython:/usr/bin/gt_ipython
$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += gt_icling:/usr/bin/gt_icling
$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += gt_shell:/usr/bin/gt_shell

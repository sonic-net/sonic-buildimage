# docker image for brcm syncd

DOCKER_SYNCD_PLATFORM_CODE = brcm
include $(PLATFORM_PATH)/../template/docker-syncd-bookworm.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD)
$(DOCKER_SYNCD_BASE)_DEPENDS += $(BRCM_XGS_SAI)
$(DOCKER_SYNCD_BASE)_DEPENDS += $(SSWSYNCD_BCMCMD) $(SSWSYNCD_DSSERVE)
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

$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += bcmsh:/usr/bin/bcmsh
$(DOCKER_SYNCD_BASE)_BASE_IMAGE_FILES += bcm_common:/usr/bin/bcm_common

# Copy the sswsyncd binaries into the docker files directory
# Define file targets for sswsyncd binaries
SSWSYNCD_BCMCMD_BIN = sswsyncd-bcmcmd.bin
$(SSWSYNCD_BCMCMD_BIN)_PATH = $(PLATFORM_PATH)/sswsyncd/sswsyncd-bcmcmd.bin
$(DOCKER_SYNCD_BASE)_FILES += $(SSWSYNCD_BCMCMD_BIN)

SSWSYNCD_DSSERVE_BIN = sswsyncd-dsserve.bin
$(SSWSYNCD_DSSERVE_BIN)_PATH = $(PLATFORM_PATH)/sswsyncd/sswsyncd-dsserve.bin
$(DOCKER_SYNCD_BASE)_FILES += $(SSWSYNCD_DSSERVE_BIN)

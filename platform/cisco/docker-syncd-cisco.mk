# docker image for syncd

DOCKER_SYNCD_PLATFORM_CODE = cisco
include $(PLATFORM_PATH)/../template/docker-syncd-bookworm.mk

# docker-syncd-bookworm.mk sets $(DOCKER_SYNCD_BASE)_PATH with '=' (lazy); pin the
# Dockerfile directory under the SONiC platform tree with ':='.
$(DOCKER_SYNCD_BASE)_PATH := $(PLATFORM_PATH)/docker-syncd-$(DOCKER_SYNCD_PLATFORM_CODE)

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD)
$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += \
    $(SYNCD_DBG) \
    $(LIBSWSSCOMMON_DBG) \
    $(LIBSAIMETADATA_DBG) \
    $(LIBSAIREDIS_DBG)

$(DOCKER_SYNCD_BASE)_FILES += $(RDB-CLI)

$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /sys:/sys:rw
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /var/run/docker-syncd:/var/run/sswsyncd
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /host/warmboot:/var/warmboot
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /var/cache/cisco:/var/cache/cisco
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /usr/lib/cisco:/usr/lib/cisco
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /opt/cisco/silicon-one:/opt/cisco/silicon-one
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /opt/cisco/data:/opt/cisco/data:ro
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /var/run/config_gen:/var/run/config_gen
$(DOCKER_SYNCD_BASE)_RUN_OPT += -e LD_LIBRARY_PATH=/usr/lib/:/usr/lib/cisco/
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /usr/release_info:/usr/release_info
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /var/dump/:/var/dump/

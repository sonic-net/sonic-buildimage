# docker image for Cisco gbsyncd

DOCKER_GBSYNCD_PLATFORM_CODE = cisco

include $(PLATFORM_PATH)/../template/docker-gbsyncd-bookworm.mk

$(DOCKER_GBSYNCD_BASE)_PATH := $(PLATFORM_PATH)/docker-gbsyncd-$(DOCKER_GBSYNCD_PLATFORM_CODE)

$(DOCKER_GBSYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_GBSYNCD_BASE)_PACKAGE_NAME = gbsyncd-$(DOCKER_GBSYNCD_PLATFORM_CODE)

$(DOCKER_GBSYNCD_BASE)_DEPENDS += $(SYNCD) \
    $(CISCO_WB_BSP_PYLIB) \
    $(LIBSSL) \
    $(GB_SAI) \
    $(YAML)

$(DOCKER_SYNCD)_DBG_DEPENDS += \
    $(SYNCD_DBG) \
    $(LIBSWSSCOMMON_DBG) \
    $(LIBSAIMETADATA_DBG) \
    $(LIBSAIREDIS_DBG)

$(DOCKER_GBSYNCD_BASE)_RUN_OPT += \
    -v /var/cache/cisco:/var/cache/cisco \
    -v /opt/cisco:/opt/cisco:ro \
    -v /sys/kernel/debug:/sys/kernel/debug:ro \
    -v /usr/lib/cisco:/usr/lib/cisco \
    -v /opt/cisco/silicon-one:/opt/cisco/silicon-one \
    -e LD_LIBRARY_PATH=/usr/lib/:/opt/cisco/lib:/usr/lib/cisco \
    -v /dev/shm:/dev/shm \
    --env "OPTICS_DEBUG_PORT"=7500

$(DOCKER_GBSYNCD_BASE)_APT_PACKAGES += \
    i2c-tools \
    libcurl4-openssl-dev \
    libjansson4 \
    libpci3 \
    libyaml

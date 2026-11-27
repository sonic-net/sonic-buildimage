# Copyright (c) 2026 Cisco Systems, Inc. and/or its affiliates.
# SPDX-License-Identifier: Apache-2.0

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

$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /sys/devices:/sys/devices:rw
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /var/run/docker-syncd:/var/run/sswsyncd
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /var/run/config_gen:/var/run/config_gen
$(DOCKER_SYNCD_BASE)_RUN_OPT += -e CISCO_PLATFORM_CAPABILITIES=$(CISCO_PLATFORM_CAPABILITIES)
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /usr/release_info:/usr/release_info
$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /var/dump/:/var/dump/

# VXR uses the TUN device and Pacific ISO when those devices are present.
$(DOCKER_SYNCD_BASE)_RUN_OPT += \$$([ -c /dev/net/tun ] && echo '--device=/dev/net/tun:/dev/net/tun')
$(DOCKER_SYNCD_BASE)_RUN_OPT += \$$([ -b /dev/sr1 ] && echo '--device=/dev/sr1:/dev/sr1:r')

# Preserve access to Cisco ASIC UIO device nodes without exposing all of /dev.
$(DOCKER_SYNCD_BASE)_RUN_OPT += \$$(for device in /dev/uio*; do \
    [ -c "\$$device" ] && echo "--device=\$$device:\$$device"; \
done)

# Allow platform_env.conf to override the Silicon One mounts for other ASICs.
# Add app_cli_client only when the host binary is present.
$(DOCKER_SYNCD_BASE)_RUN_OPT += \$${CISCO_SYNCD_PLATFORM_OPT:-\
    -v /var/cache/cisco:/var/cache/cisco \
    -v /usr/lib/cisco:/usr/lib/cisco \
    -v /opt/cisco/silicon-one:/opt/cisco/silicon-one \
    -v /opt/cisco/data:/opt/cisco/data:ro \
    -v /opt/cisco/syncd:/opt/cisco/syncd:ro \
    \$$([ -f /usr/bin/app_cli_client ] && echo '-v /usr/bin/app_cli_client:/usr/bin/app_cli_client:ro') \
    -e LD_LIBRARY_PATH=/usr/lib/:/usr/lib/cisco/}

# Copyright (c) 2026 Cisco Systems, Inc. and/or its affiliates.
# SPDX-License-Identifier: Apache-2.0

#
# DOCKER_PLATFORM_MONITOR comes from rules/docker-platform-monitor.mk which is
# included before the platform rules.mk
#
$(DOCKER_PLATFORM_MONITOR)_PYTHON_WHEELS += $(CISCO_PACIFIC_WHEEL)

# Additional platform specific libraries.
$(DOCKER_PLATFORM_MONITOR)_DEPENDS += \
    $(CISCO_WB_BSP_PYLIB) \
    $(LIBSSL) \
    $(YAML) \
    $(IPUTILS-PING)

CISCO_BSP_PYTHON_MODULE_PATH := /opt/cisco/lib/python

# .bundled/ contains the third-party shared libraries (libyaml-cpp, libglog,
# libunwind, libgflags, ...) that match the trixie pmon container and host.
$(DOCKER_PLATFORM_MONITOR)_RUN_OPT += \
    -v /var/cache/cisco:/var/cache/cisco \
    -v /opt/cisco:/opt/cisco:ro \
    -v /usr/lib/cisco:/usr/lib/cisco \
    -v /opt/cisco/lib/.bundled:/opt/cisco/lib/.bundled:ro \
    -v $(CISCO_BSP_PYTHON_MODULE_PATH):$(CISCO_BSP_PYTHON_MODULE_PATH) \
    -v /opt/cisco/silicon-one:/opt/cisco/silicon-one \
    -e LD_LIBRARY_PATH=/opt/cisco/lib/.bundled:/usr/lib:/usr/lib/cisco:/opt/cisco/lib \
    -e PYTHONPATH=$(CISCO_BSP_PYTHON_MODULE_PATH):$$PYTHONPATH \
    -v /dev/shm:/dev/shm \
    --env "OPTICS_DEBUG_PORT"=7500

$(DOCKER_PLATFORM_MONITOR)_APT_PACKAGES += \
    i2c-tools \
    libcurl4-openssl-dev \
    libjansson4 \
    libpci3

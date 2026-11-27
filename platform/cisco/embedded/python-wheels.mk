# Copyright (c) 2026 Cisco Systems, Inc. and/or its affiliates.
# SPDX-License-Identifier: Apache-2.0

# Build pmon's platform wheel from only the Python package in the prebuilt
# module. The Debian package also contains host services and platform files
# which should not be installed into the pmon image.
CISCO_EMBEDDED_PLATFORM_WHEEL := sonic_platform-1.0-py3-none-any.whl
export CISCO_EMBEDDED_PLATFORM_MODULE_DEB := $(abspath $(DEBS_PATH)/$(CISCO_8000_PLATFORM_MODULE))

$(CISCO_EMBEDDED_PLATFORM_WHEEL)_SRC_PATH := $(PLATFORM_SRC_PATH)
$(CISCO_EMBEDDED_PLATFORM_WHEEL)_DEPENDS = $(SONIC_PY_COMMON_PY3) $(SONIC_PLATFORM_COMMON_PY3) $(SONIC_CONFIG_ENGINE_PY3)
$(CISCO_EMBEDDED_PLATFORM_WHEEL)_TEST = n
$(CISCO_EMBEDDED_PLATFORM_WHEEL)_PYTHON_VERSION = 3
SONIC_PYTHON_WHEELS += $(CISCO_EMBEDDED_PLATFORM_WHEEL)

# Make the source deb available before the generic wheel rule runs. Extract it
# in setup.py instead of installing its host services and package dependencies
# into the build container.
$(PYTHON_WHEELS_PATH)/$(CISCO_EMBEDDED_PLATFORM_WHEEL): $(DEBS_PATH)/$(CISCO_8000_PLATFORM_MODULE)

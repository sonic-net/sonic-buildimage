# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# Platform packages are published by sonic-platform and sonic-modules.
CISCO_8000_PLATFORM_MODULE := \
    sonic-platform-cisco-8000_$(CISCO_8000_PLATFORM_MODULE_VERSION)_amd64.deb
$(eval $(call add_prebuilt_debs,$(CISCO_8000_PLATFORM_MODULE),$(CISCO_PLATFORM_MODULE_URL),$(CISCO_PLATFORM_API_REPO)))
$(CISCO_8000_PLATFORM_MODULE)_PLATFORM := $(CISCO_8000_PLATFORMS)

CISCO_8000_KMOD := \
    sonic-platform-cisco-8000-kmod_$(CISCO_8000_PLATFORM_MODULE_VERSION)_amd64.deb
$(eval $(call add_prebuilt_debs,$(CISCO_8000_KMOD),$(CISCO_KMOD_URL),$(CISCO_MODULES_API_REPO)))
$(CISCO_8000_KMOD)_PLATFORM := $(CISCO_8000_PLATFORMS)

CISCO_8000_S1_Q200 := \
    sonic-platform-cisco-8000-s1-q200_$(CISCO_8000_PLATFORM_MODULE_VERSION)_amd64.deb
$(eval $(call add_prebuilt_debs,$(CISCO_8000_S1_Q200),$(CISCO_S1_Q200_URL),$(CISCO_MODULES_API_REPO)))
$(CISCO_8000_S1_Q200)_PLATFORM := $(CISCO_PLATFORMS_GIBRALTAR)

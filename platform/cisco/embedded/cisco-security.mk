# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

CISCO_SECURITY := cisco-security_$(CISCO_SECURITY_VERSION)_amd64.deb
$(eval $(call add_prebuilt_debs,$(CISCO_SECURITY),$(CISCO_SECURITY_URL)))
$(CISCO_SECURITY)_PLATFORM := $(CISCO_8000_PLATFORMS)

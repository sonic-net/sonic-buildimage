# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

CISCO_WB_BSP2 := cisco-wb-bsp2_$(CISCO_WB_BSP2_VERSION)_amd64.deb
$(eval $(call add_prebuilt_debs,$(CISCO_WB_BSP2),$(CISCO_WB_BSP2_URL)))
$(CISCO_WB_BSP2)_PLATFORM := $(CISCO_8000_PLATFORMS)

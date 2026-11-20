# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

CISCO_WB_FPD := \
    cisco-wb-fpd-util_$(CISCO_WB_FPD_VERSION)_amd64.deb \
    cisco-wb-fpd_$(CISCO_WB_FPD_VERSION)_all.deb

$(foreach deb,$(CISCO_WB_FPD), \
    $(eval $(call add_prebuilt_debs,$(deb),$(CISCO_WB_FPD_URL))))

# The large FPD package is a normal release asset; it is not stored in Git
# because it exceeds GitHub's regular blob-size limit.
CISCO_WB_FPD_LARGE := cisco-wb-fpd_$(CISCO_WB_FPD_VERSION)_all.deb
CISCO_WB_FPD_LARGE_URL ?= $(CISCO_WB_FPD_URL)
$(CISCO_WB_FPD_LARGE)_URL := $(call cisco_package_url,$(CISCO_WB_FPD_LARGE),$(CISCO_WB_FPD_LARGE_URL))

$(foreach deb,$(CISCO_WB_FPD), \
    $(eval $(deb)_PLATFORM := $(CISCO_8000_PLATFORMS)))

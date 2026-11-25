# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# MTD tooling used by the Cisco platform firmware paths.
MTD-UTILS := mtd-utils_2.0.1-1_amd64.deb
$(MTD-UTILS)_PLATFORM := $(CISCO_8000_PLATFORMS)
$(MTD-UTILS)_URL := http://archive.debian.org/debian/pool/main/m/mtd-utils/$(MTD-UTILS)

SONIC_ONLINE_DEBS += $(MTD-UTILS)
SONIC_BUSTER_DEBS += $(MTD-UTILS)

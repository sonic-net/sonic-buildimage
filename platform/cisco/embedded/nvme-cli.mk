# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# NVMe tooling is used by the Cisco SSD and firmware-management paths.
NVME-CLI := nvme-cli_1.7-1_amd64.deb
$(NVME-CLI)_PLATFORM := $(CISCO_8000_PLATFORMS)
$(NVME-CLI)_URL := http://archive.debian.org/debian/pool/main/n/nvme-cli/$(NVME-CLI)

SONIC_ONLINE_DEBS += $(NVME-CLI)
SONIC_BUSTER_DEBS += $(NVME-CLI)

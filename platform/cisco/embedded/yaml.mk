# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# Include libyaml as part of the ONIE image, matching platform-cisco-8000.
YAML := libyaml-0-2_0.2.2-1_amd64.deb
$(YAML)_PLATFORM := $(CISCO_8000_PLATFORMS)
$(YAML)_URL := http://archive.debian.org/debian/pool/main/liby/libyaml/$(YAML)

SONIC_ONLINE_DEBS += $(YAML)

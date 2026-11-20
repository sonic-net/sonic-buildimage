# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# Embedded platforms use the SDK SAI artifact.
SDK_SAI := \
    cisco-gibraltar_$(CISCO_Q200_SILICON_ONE_VERSION)_amd64.deb

# SDK SAI dependency aliases.
GIBRALTAR_SAI := $(SDK_SAI)
GB_SAI := $(SDK_SAI)
$(eval $(call add_prebuilt_debs,$(SDK_SAI),$(CISCO_SDK_URL)))
$(SDK_SAI)_PLATFORM := $(CISCO_PLATFORMS_GIBRALTAR)

# The shared RPC rules build against SONiC's SAI source headers. Keep only the
# Cisco runtime SAI as the selected implementation for linking and runtime.
CISCO_BUILD_SAI := $(SDK_SAI)

# Copyright (c) 2026 Cisco Systems, Inc. and/or its affiliates.
# SPDX-License-Identifier: Apache-2.0

# Prefer the vendor checkout. The embedded tree contains the supported
# 8101/8102 device data and prebuilt package rules, so an inaccessible vendor
# repository does not prevent the Cisco build from starting.
CISCO_VENDOR_PLATFORM_PATH := $(PLATFORM_PATH)/src
CISCO_EMBEDDED_PLATFORM_PATH := $(PLATFORM_PATH)/embedded
ifneq ($(wildcard $(CISCO_VENDOR_PLATFORM_PATH)/rules.mk),)
PLATFORM_SRC_PATH := $(CISCO_VENDOR_PLATFORM_PATH)
CISCO_PLATFORM_SOURCE_MODE := source
else
PLATFORM_SRC_PATH := $(CISCO_EMBEDDED_PLATFORM_PATH)
CISCO_PLATFORM_SOURCE_MODE := embedded
endif
export PLATFORM_SRC_PATH

include $(PLATFORM_SRC_PATH)/rules.mk

# The vendor rules are included through PLATFORM_SRC_PATH, but their Python
# wheel sources are relative to the vendor checkout, not PLATFORM_PATH.
ifeq ($(CISCO_PLATFORM_SOURCE_MODE),source)
$(CISCO_PACIFIC_WHEEL)_SRC_PATH := $(PLATFORM_SRC_PATH)/$(CISCO_PACIFIC_WHEEL_REL_PATH)
endif

# Comma-separated capabilities supported by this platform. Source-backed
# platforms retain the current DShell behavior unless they override this.
CISCO_PLATFORM_CAPABILITIES ?= dshell

# The vendor rules already include these shared Docker fragments through
# PLATFORM_PATH. The embedded rules do not, so include them only for fallback.
ifeq ($(CISCO_PLATFORM_SOURCE_MODE),embedded)
include $(PLATFORM_PATH)/docker-orchagent.mk
include $(PLATFORM_PATH)/docker-platform-monitor.mk
include $(PLATFORM_PATH)/docker-syncd-cisco.mk
include $(PLATFORM_PATH)/docker-syncd-cisco-rpc.mk
include $(PLATFORM_PATH)/docker-gbsyncd-cisco.mk
endif

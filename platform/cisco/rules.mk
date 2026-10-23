# Copyright (c) 2026 Cisco Systems, Inc. and/or its affiliates.
# SPDX-License-Identifier: Apache-2.0

# platform-cisco repo sources under $(PLATFORM_SRC_PATH)
PLATFORM_SRC_PATH := $(PLATFORM_PATH)/src
export PLATFORM_SRC_PATH

include $(PLATFORM_SRC_PATH)/rules.mk

# Comma-separated capabilities supported by this platform. Source-backed
# platforms retain the current DShell behavior unless they override this.
CISCO_PLATFORM_CAPABILITIES ?= dshell
export CISCO_PLATFORM_CAPABILITIES

include $(PLATFORM_PATH)/docker-platform-monitor.mk
include $(PLATFORM_PATH)/docker-syncd-cisco.mk
include $(PLATFORM_PATH)/docker-syncd-cisco-rpc.mk
include $(PLATFORM_PATH)/docker-gbsyncd-cisco.mk

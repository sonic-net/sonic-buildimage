# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# Cisco embedded platform package rules.

PLATFORM_SRC_PATH ?= $(PLATFORM_PATH)/embedded

include $(PLATFORM_SRC_PATH)/versions.mk
include $(PLATFORM_SRC_PATH)/platform-rules.mk

# Platform setup: platform.conf and device data.
rc := $(shell $(PLATFORM_PATH)/platform_setup.sh >/dev/null; echo $$?)
ifneq ($(rc),0)
$(error Cisco embedded platform_setup.sh returned error $(rc))
endif

include $(PLATFORM_SRC_PATH)/pyudev.mk
include $(PLATFORM_SRC_PATH)/cisco-wb-bsp.mk
include $(PLATFORM_SRC_PATH)/cisco-wb-bsp2.mk
include $(PLATFORM_SRC_PATH)/cisco-wb-fpd.mk
include $(PLATFORM_SRC_PATH)/cisco-security.mk
include $(PLATFORM_SRC_PATH)/sdk-sai.mk
include $(PLATFORM_SRC_PATH)/yaml.mk
include $(PLATFORM_SRC_PATH)/nvme-cli.mk
include $(PLATFORM_SRC_PATH)/mtd-utils.mk
include $(PLATFORM_SRC_PATH)/platform-modules-cisco.mk
include $(PLATFORM_SRC_PATH)/python-wheels.mk
include $(PLATFORM_SRC_PATH)/onie-image.mk

ifeq ($(ENABLE_SYNCD_RPC),y)
include $(PLATFORM_PATH)/libsaithrift-dev.mk
# Replace the obsolete Thrift 0.11 development package injected by the
# generic syncd RPC rule with the SAI Thrift package used by Cisco builds.
$(SYNCD)_DEPENDS := $(filter-out $(LIBTHRIFT_DEV),$($(SYNCD)_DEPENDS))
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV)
endif

# The SDK SAI is the only SAI used by the embedded platform set.
$(LIBSAIREDIS)_DEPENDS += $(SDK_SAI)
$(SYNCD)_DEPENDS += $(SDK_SAI)

SONIC_ALL += $(SONIC_ONIE_IMAGE) $(DOCKER_FPM)

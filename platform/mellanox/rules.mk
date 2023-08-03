#
# Copyright (c) 2016-2023 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
include $(PLATFORM_PATH)/sdk.mk
include $(PLATFORM_PATH)/fw.mk
include $(PLATFORM_PATH)/mft.mk
include $(PLATFORM_PATH)/mft-fwtrace-cfg.mk
include $(PLATFORM_PATH)/mlnx-sai.mk
include $(PLATFORM_PATH)/hw-management.mk
include $(PLATFORM_PATH)/mlnx-platform-api.mk
include $(PLATFORM_PATH)/docker-syncd-mlnx.mk
include $(PLATFORM_PATH)/docker-syncd-mlnx-rpc.mk
include $(PLATFORM_PATH)/docker-saiserver-mlnx.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/libsaithrift-dev.mk
include $(PLATFORM_PATH)/mlnx-ffb.mk
include $(PLATFORM_PATH)/issu-version.mk
include $(PLATFORM_PATH)/mlnx-onie-fw-update.mk
include $(PLATFORM_PATH)/mlnx-ssd-fw-update.mk
include $(PLATFORM_PATH)/iproute2.mk
include $(PLATFORM_PATH)/install-pending-fw.mk
include $(PLATFORM_PATH)/integration-scripts.mk

SONIC_ALL += $(SONIC_ONE_IMAGE) \
             $(DOCKER_FPM)

# Inject mlnx sai into syncd
$(SYNCD)_DEPENDS += $(MLNX_SAI)
$(SYNCD)_UNINSTALLS += $(MLNX_SAI)

ifeq ($(ENABLE_SYNCD_RPC),y)
$(SYNCD)_DEPENDS += $(LIBSAITHRIFT_DEV)
endif

# Runtime dependency on mlnx sai is set only for syncd
$(SYNCD)_RDEPENDS += $(MLNX_SAI)

# Inject mlnx sdk libs to platform monitor
$(DOCKER_PLATFORM_MONITOR)_DEPENDS += $(APPLIBS) $(SX_COMPLIB) $(SXD_LIBS) $(SX_GEN_UTILS) $(PYTHON_SDK_API) $(APPLIBS_DEV) $(SX_COMPLIB_DEV) $(SXD_LIBS_DEV) $(SX_GEN_UTILS_DEV)

# Force the target bootloader for mellanox platforms to grub regardless of arch
TARGET_BOOTLOADER = grub

# location for the platform specific external kernel patches
override EXTERNAL_KERNEL_PATCH_LOC := $(BUILD_WORKDIR)/$(PLATFORM_PATH)/non-upstream-patches/

export SONIC_BUFFER_MODEL=dynamic

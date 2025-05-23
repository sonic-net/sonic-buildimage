#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

RECIPE_DIR = recipes

override TARGET_BOOTLOADER=grub

include $(PLATFORM_PATH)/$(RECIPE_DIR)/bluefield-soc.mk
include $(PLATFORM_PATH)/$(RECIPE_DIR)/mft.mk
include $(PLATFORM_PATH)/$(RECIPE_DIR)/fw.mk
include $(PLATFORM_PATH)/$(RECIPE_DIR)/dpu-sai.mk
include $(PLATFORM_PATH)/$(RECIPE_DIR)/sdk.mk
include $(PLATFORM_PATH)/$(RECIPE_DIR)/platform-api.mk
include $(PLATFORM_PATH)/$(RECIPE_DIR)/docker-syncd-bluefield.mk
include $(PLATFORM_PATH)/$(RECIPE_DIR)/installer-image.mk
include $(PLATFORM_PATH)/$(RECIPE_DIR)/component-versions.mk

# Inject DPU sai into syncd
$(SYNCD)_DEPENDS += $(DPU_SAI)
$(SYNCD)_UNINSTALLS += $(DPU_SAI)

# Runtime dependency on DPU sai is set only for syncd
$(SYNCD)_RDEPENDS += $(DPU_SAI)

# Inject mft into platform monitor
$(DOCKER_PLATFORM_MONITOR)_DEPENDS += $(MFT)
$(DOCKER_PLATFORM_MONITOR)_DEPENDS += $(MLXBF_BOOTCTL_DEB)

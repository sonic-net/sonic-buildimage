#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

# NVIDIA hw-mgmt for BMC (Aspeed platform)

include platform/mellanox/hw-management.mk

# Override SRC_PATH: the include above set it to $(PLATFORM_PATH)/hw-management
$(MLNX_HW_MANAGEMENT)_SRC_PATH = platform/mellanox/hw-management
$(MLNX_HW_MANAGEMENT_BMC)_SRC_PATH = platform/mellanox/hw-management
$(MLNX_HW_MANAGEMENT_BMC)_PLATFORM = arm64-aspeed_nvidia_ast2700_bmc-r0
SONIC_MAKE_DEBS += $(MLNX_HW_MANAGEMENT_BMC)

# Point to mellanox platform directory where hw-mgmt source and integration scripts live.
MLNX_PLATFORM_PATH = platform/mellanox

include platform/mellanox/integration-scripts.mk

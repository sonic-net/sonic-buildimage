#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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

# NVIDIA BMC Platform modules

ASPEED_NVIDIA_AST2700_BMC_PLATFORM_MODULE = sonic-platform-aspeed-nvidia-ast2700-bmc_1.0_arm64.deb
$(ASPEED_NVIDIA_AST2700_BMC_PLATFORM_MODULE)_SRC_PATH = $(PLATFORM_PATH)/sonic-platform-modules-nvidia-bmc
$(ASPEED_NVIDIA_AST2700_BMC_PLATFORM_MODULE)_PLATFORM = arm64-aspeed_nvidia_ast2700_bmc-r0
SONIC_DPKG_DEBS += $(ASPEED_NVIDIA_AST2700_BMC_PLATFORM_MODULE)

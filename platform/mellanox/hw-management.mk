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
# Mellanox HW Management

MLNX_HW_MANAGEMENT_VERSION = 7.0030.1011

export MLNX_HW_MANAGEMENT_VERSION

MLNX_HW_MANAGEMENT = hw-management_1.mlnx.$(MLNX_HW_MANAGEMENT_VERSION)_$(CONFIGURED_ARCH).deb
$(MLNX_HW_MANAGEMENT)_SRC_PATH = $(PLATFORM_PATH)/hw-management
$(MLNX_HW_MANAGEMENT)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
SONIC_MAKE_DEBS += $(MLNX_HW_MANAGEMENT)

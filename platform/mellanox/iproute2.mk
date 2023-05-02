#
# Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES.
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

IPROUTE2_MLNX_VERSION = 5.10.0
IPROUTE2_MLNX_VERSION_FULL = $(IPROUTE2_MLNX_VERSION)-4~bpo10+1

export IPROUTE2_MLNX_VERSION
export IPROUTE2_MLNX_VERSION_FULL

IPROUTE2_MLNX = iproute2-mlnx_$(IPROUTE2_MLNX_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(IPROUTE2_MLNX)_SRC_PATH = $(PLATFORM_PATH)/iproute2
SONIC_MAKE_DEBS += $(IPROUTE2_MLNX)

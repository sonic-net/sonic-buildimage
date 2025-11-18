#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
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

MLNX_RSHIM_DRIVER_VERSION = 2.3.8
MLNX_RSHIM_ASSETS_GITHUB_URL = https://github.com/Mellanox/sonic-bluefield-packages
MLNX_RSHIM_ASSETS_RELEASE_TAG = rshim-$(MLNX_RSHIM_DRIVER_VERSION)-$(BLDENV)-$(CONFIGURED_ARCH)
MLNX_RSHIM_ASSETS_URL = $(MLNX_RSHIM_ASSETS_GITHUB_URL)/releases/download/$(MLNX_RSHIM_ASSETS_RELEASE_TAG)

MLNX_RSHIM = rshim_${MLNX_RSHIM_DRIVER_VERSION}_${CONFIGURED_ARCH}.deb
$(MLNX_RSHIM)_SRC_PATH = $(PLATFORM_PATH)/rshim
$(MLNX_RSHIM)_URL = $(MLNX_RSHIM_ASSETS_URL)/$(MLNX_RSHIM)
$(MLNX_RSHIM)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)

MLNX_BSD_RSHIM_SRC_BASE_URL =

export MLNX_RSHIM MLNX_BSD_RSHIM_SRC_BASE_URL MLNX_RSHIM_DRIVER_VERSION

ifneq ($(MLNX_BSD_RSHIM_SRC_BASE_URL), )
SONIC_MAKE_DEBS += $(MLNX_RSHIM)
else
SONIC_ONLINE_DEBS += $(MLNX_RSHIM)
endif

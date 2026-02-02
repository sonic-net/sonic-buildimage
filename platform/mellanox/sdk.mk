#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2021-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
MLNX_SDK_VERSION = 4.8.2122
MLNX_SDK_ISSU_VERSION = 101

MLNX_SDK_DRIVERS_GITHUB_URL = https://github.com/Mellanox/Spectrum-SDK-Drivers
MLNX_ASSETS_GITHUB_URL = https://github.com/Mellanox/Spectrum-SDK-Drivers-SONiC-Bins
MLNX_SDK_ASSETS_RELEASE_TAG = sdk-$(MLNX_SDK_VERSION)-$(BLDENV)-$(CONFIGURED_ARCH)
MLNX_SDK_ASSETS_URL = $(MLNX_ASSETS_GITHUB_URL)/releases/download/$(MLNX_SDK_ASSETS_RELEASE_TAG)
MLNX_SDK_DEB_VERSION = $(subst -,.,$(subst _,.,$(MLNX_SDK_VERSION)))

# Place here URL where alternate SDK assets exist
MLNX_SDK_ASSETS_BASE_URL =

# Place here URL where SDK sources exist
MLNX_SDK_SOURCE_BASE_URL =

# Use alternate assets URL if provided
ifneq ($(MLNX_SDK_ASSETS_BASE_URL), )
MLNX_SDK_ASSETS_URL = $(MLNX_SDK_ASSETS_BASE_URL)
endif

# Use source build if no assets URL is provided but source URL is available
SDK_FROM_SRC = $(if $(MLNX_SDK_ASSETS_BASE_URL),n,$(if $(MLNX_SDK_SOURCE_BASE_URL),y,n))

export MLNX_SDK_SOURCE_BASE_URL MLNX_SDK_VERSION MLNX_SDK_ISSU_VERSION MLNX_SDK_DEB_VERSION MLNX_ASSETS_GITHUB_URL MLNX_SDK_DRIVERS_GITHUB_URL

MLNX_SDK_RDEBS += $(SYSSDK)
MLNX_SDK_DEBS += $(SYSSDK_DEV)
MLNX_SDK_DBG_DEBS += $(SYSSDK_DBGSYM)

SYSSDK = sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(SYSSDK)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sys-sdk
$(SYSSDK)_DEPENDS += $(LIBNL3_DEV) $(LIBNL_GENL3_DEV)
$(SYSSDK)_RDEPENDS += $(LIBNL3) $(LIBNL_GENL3)
SYSSDK_DEV = sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH)-dev.deb
$(eval $(call add_derived_package,$(SYSSDK),$(SYSSDK_DEV)))
SYSSDK_DBGSYM = sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH)-dbgsym.ddeb
ifeq ($(SDK_FROM_SRC),y)
$(eval $(call add_derived_package,$(SYSSDK),$(SYSSDK_DBGSYM)))
endif


#packages that are required for runtime only

SX_KERNEL = sx-kernel_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(SX_KERNEL)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
$(SX_KERNEL)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sx-kernel
SX_KERNEL_DEV = sx-kernel-dev_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SX_KERNEL),$(SX_KERNEL_DEV)))

define make_url
	$(1)_URL = $(MLNX_SDK_ASSETS_URL)/$(1)

endef

$(eval $(foreach deb,$(MLNX_SDK_DEBS) $(MLNX_SDK_RDEBS) $(PYTHON_SDK_API),$(call make_url,$(deb))))

SONIC_MAKE_DEBS += $(SX_KERNEL)

ifeq ($(SDK_FROM_SRC), y)
SONIC_MAKE_DEBS += $(MLNX_SDK_RDEBS) $(PYTHON_SDK_API)
else
SONIC_ONLINE_DEBS += $(MLNX_SDK_RDEBS) $(PYTHON_SDK_API)
endif

mlnx-sdk-packages: $(addprefix $(DEBS_PATH)/, $(MLNX_SDK_RDEBS) $(PYTHON_SDK_API) $(SX_KERNEL))

SONIC_PHONY_TARGETS += mlnx-sdk-packages

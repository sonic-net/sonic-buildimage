#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2016-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

MLNX_SDK_VERSION = 4.10.1090
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

SYSSDK = sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(SYSSDK)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sys-sdk
$(SYSSDK)_RDEPENDS += $(LIBNL3) $(LIBNL_GENL3)
SYSSDK_DEV = sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH)-dev.deb
SYSSDK_DBGSYM = sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH)-dbgsym.ddeb


#packages that are required for runtime only

SX_KERNEL = sx-kernel_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(SX_KERNEL)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
$(SX_KERNEL)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sx-kernel

define make_url
	$(1)_URL = $(MLNX_SDK_ASSETS_URL)/$(1)

endef

SONIC_MAKE_DEBS += $(SX_KERNEL)

# Which extra artifacts exist (-dev headers, dbgsym) depends on how the SDK is
# obtained, following the same strategy as mlnx-sai.mk: each branch declares
# only what it can actually deliver, and an extra is attached to the runtime
# deb only when a single recipe run emits both.
#
# Built from source (SDK_FROM_SRC=y):
#   One cpack run writes the runtime deb, -dev and, unless nostrip is set, the
#   dbgsym, so both extras are registered as derived packages of the runtime
#   deb. sys-sdk is packaged by CPack rather than debhelper and cannot pick up
#   DEB_BUILD_OPTIONS by itself, so sdk-src/sys-sdk/Makefile maps nostrip to
#   CPACK_DEBIAN_DEBUGINFO_PACKAGE=OFF; that is why SPLIT_DBGSYM=n has to skip
#   the derived dbgsym here as well. _DEPENDS is the compile set: libnl
#   headers; -dev still pulls the runtime libnl via its own _DEPENDS.
#
# Downloaded (SDK_FROM_SRC=n):
#   All three are independent release assets, so each becomes its own online
#   deb with its own URL. That keeps the extras lazy: -dev is fetched only when
#   something compiles against the SDK (SAI built from source), the dbgsym only
#   when the -dbg image is built. Deriving them from the runtime deb instead
#   would drag both down on every build. -dev must still be installed after the
#   runtime deb, which $(SYSSDK_DEV)_DEPENDS records without turning it into a
#   derived package. _DEPENDS is the dpkg -i set: runtime libnl, not -dev, so
#   a full image that only installs the runtime SDK does not fetch libnl
#   headers.
#
# MLNX_SDK_DBG_DEBS is what docker-syncd-mlnx.mk appends to the -dbg image's
# DBG_DEPENDS; it stays empty when no dbgsym package exists.
ifeq ($(SDK_FROM_SRC), y)
$(SYSSDK)_DEPENDS += $(LIBNL3_DEV) $(LIBNL_GENL3_DEV)
SONIC_MAKE_DEBS += $(MLNX_SDK_RDEBS)
$(eval $(call add_derived_package,$(SYSSDK),$(SYSSDK_DEV)))
ifeq ($(SPLIT_DBGSYM),y)
$(eval $(call add_derived_package,$(SYSSDK),$(SYSSDK_DBGSYM)))
MLNX_SDK_DBG_DEBS += $(SYSSDK_DBGSYM)
endif
else
$(SYSSDK)_DEPENDS += $(LIBNL3) $(LIBNL_GENL3)
SONIC_ONLINE_DEBS += $(MLNX_SDK_RDEBS) $(SYSSDK_DEV) $(SYSSDK_DBGSYM)
$(SYSSDK_DEV)_DEPENDS += $(SYSSDK)
$(eval $(foreach deb,$(MLNX_SDK_RDEBS) $(SYSSDK_DEV) $(SYSSDK_DBGSYM),$(call make_url,$(deb))))
MLNX_SDK_DBG_DEBS += $(SYSSDK_DBGSYM)
endif

mlnx-sdk-packages: $(addprefix $(DEBS_PATH)/, $(MLNX_SDK_RDEBS) $(SX_KERNEL))

SONIC_PHONY_TARGETS += mlnx-sdk-packages

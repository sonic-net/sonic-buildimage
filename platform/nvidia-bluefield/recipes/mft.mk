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

MFT_VERSION = 4.34.0
MFT_REVISION = 145

MFT_INTERNAL_SOURCE_BASE_URL =

ifneq ($(MFT_INTERNAL_SOURCE_BASE_URL), )
MFT_FROM_INTERNAL = y
else
MFT_FROM_INTERNAL = n
endif

export MFT_VERSION MFT_REVISION MFT_FROM_INTERNAL MFT_INTERNAL_SOURCE_BASE_URL

MFT = mft_$(MFT_VERSION)-$(MFT_REVISION)_arm64.deb
$(MFT)_SRC_PATH = $(PLATFORM_PATH)/mft

MFT_OEM = mft-oem_$(MFT_VERSION)-$(MFT_REVISION)_arm64.deb
$(eval $(call add_derived_package,$(MFT),$(MFT_OEM)))

KERNEL_MFT_DKMS = kernel-mft-dkms_$(MFT_VERSION)-$(MFT_REVISION)_all.deb
$(eval $(call add_derived_package,$(MFT),$(KERNEL_MFT_DKMS)))
$(KERNEL_MFT_DKMS)_DEPENDS = $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
# Note: Restrict the DKMS package to compile the driver only against the target kernel headers
$(KERNEL_MFT_DKMS)_DEB_INSTALL_OPTS = "KVERSION=$(KVERSION)"

BUILD_ARCH := $(shell dpkg-architecture -qDEB_BUILD_ARCH)
KERNEL_MFT = kernel-mft-dkms-modules-$(KVERSION)_$(MFT_VERSION)_$(BUILD_ARCH).deb
$(KERNEL_MFT)_SRC_PATH = $(PLATFORM_PATH)/mft
$(KERNEL_MFT)_DEPENDS += $(KERNEL_MFT_DKMS)

SONIC_MAKE_DEBS += $(MFT) $(KERNEL_MFT)

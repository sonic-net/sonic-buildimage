#
# Copyright (c) 2016-2022 NVIDIA CORPORATION & AFFILIATES.
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
# Mellanox SAI

MFT_VERSION = 4.22.1
MFT_REVISION = 15

MLNX_MFT_INTERNAL_SOURCE_BASE_URL =

ifneq ($(MLNX_MFT_INTERNAL_SOURCE_BASE_URL), )
MFT_FROM_INTERNAL = y
else
MFT_FROM_INTERNAL = n
endif

export MFT_VERSION MFT_REVISION MFT_FROM_INTERNAL MLNX_MFT_INTERNAL_SOURCE_BASE_URL

MFT = mft_$(MFT_VERSION)-$(MFT_REVISION)_$(CONFIGURED_ARCH).deb
$(MFT)_SRC_PATH = $(PLATFORM_PATH)/mft
SONIC_MAKE_DEBS += $(MFT)

$(MFT)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)

KERNEL_MFT = kernel-mft-dkms-modules-$(KVERSION)_$(MFT_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(MFT),$(KERNEL_MFT)))

MFT_OEM = mft-oem_$(MFT_VERSION)-$(MFT_REVISION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(MFT),$(MFT_OEM)))

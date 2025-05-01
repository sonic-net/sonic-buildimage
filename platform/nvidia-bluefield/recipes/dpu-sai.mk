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

DPU_SAI_VERSION = SAIBuild0.0.41.0

# Place here URL where SAI sources exist
DPU_SAI_SOURCE_BASE_URL=

export DPU_SAI_VERSION DPU_SAI_SOURCE_BASE_URL

define make_url_sai
	$(1)_URL="https://github.com/Mellanox/sonic-bluefield-packages/releases/download/dpu-sai-$(DPU_SAI_VERSION)-$(BLDENV)/$(1)"

endef

DPU_SAI = mlnx-sai_1.mlnx.$(DPU_SAI_VERSION)_arm64.deb
$(DPU_SAI)_SRC_PATH = $(PLATFORM_PATH)/dpu-sai
$(DPU_SAI)_DEPENDS = $(SDN_APPL)
$(DPU_SAI)_RDEPENDS = $(SDN_APPL)
$(eval $(call add_conflict_package,$(DPU_SAI),$(LIBSAIVS_DEV)))
DPU_SAI_DBGSYM = mlnx-sai-dbgsym_1.mlnx.$(DPU_SAI_VERSION)_arm64.deb
$(eval $(call add_derived_package,$(DPU_SAI),$(DPU_SAI_DBGSYM)))

ifneq ($(DPU_SAI_SOURCE_BASE_URL), )
SONIC_MAKE_DEBS += $(DPU_SAI)
else
$(eval $(foreach deb,$(DPU_SAI) $(DPU_SAI_DBGSYM),$(call make_url_sai,$(deb))))
SONIC_ONLINE_DEBS += $(DPU_SAI)
endif


export DPU_SAI
export DPU_SAI_DBGSYM

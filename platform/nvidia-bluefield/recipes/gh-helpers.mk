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

BF_ASSETS_GITHUB_REPO = nvidia-sonic/sonic-bluefield-packages
SDK_ASSETS_GITHUB_TOKEN = 

export BF_ASSETS_GITHUB_REPO SDK_ASSETS_GITHUB_TOKEN

define get_sdk_version_file_gh
$(shell $(PLATFORM_PATH)/recipes/get_sdk_version_file_gh.sh sdk-$(SDK_VERSION)-$(BLDENV) $(1) $(SDK_ASSETS_GITHUB_TOKEN))
endef

define get_sdk_asset_id_gh
$(shell $(PLATFORM_PATH)/recipes/get_package_gh_asset_id.sh sdk-$(SDK_VERSION)-$(BLDENV) $(1) $(SDK_ASSETS_GITHUB_TOKEN))
endef

define get_sai_asset_id_gh
$(shell $(PLATFORM_PATH)/recipes/get_package_gh_asset_id.sh sai-$(DPU_SAI_VERSION)-$(BLDENV) $(1) $(SDK_ASSETS_GITHUB_TOKEN))
endef

define get_fw_asset_id_gh
$(shell $(PLATFORM_PATH)/recipes/get_package_gh_asset_id.sh fw-$(BF3_FW_VERSION) $(1) $(SDK_ASSETS_GITHUB_TOKEN))
endef

define get_bfsoc_asset_id_gh
$(shell $(PLATFORM_PATH)/recipes/get_package_gh_asset_id.sh bfsoc-$(BFSOC_VERSION)-$(BFSOC_REVISION)-$(BLDENV) $(1) $(SDK_ASSETS_GITHUB_TOKEN))
endef

define make_url_sdk
	$(1)_URL="https://api.github.com/repos/$(BF_ASSETS_GITHUB_REPO)/releases/assets/$(call get_sdk_asset_id_gh, $(1))"
	$(1)_CURL_OPTIONS=-H "Accept: application/octet-stream" -H "Authorization: token $(SDK_ASSETS_GITHUB_TOKEN)"
	$(1)_SKIP_VERSION=y

endef

define make_url_sai
	$(1)_URL="https://api.github.com/repos/$(BF_ASSETS_GITHUB_REPO)/releases/assets/$(call get_sai_asset_id_gh, $(1))"
	$(1)_CURL_OPTIONS=-H "Accept: application/octet-stream" -H "Authorization: token $(SDK_ASSETS_GITHUB_TOKEN)"
	$(1)_SKIP_VERSION=y

endef

define make_url_fw
	$(1)_URL="https://api.github.com/repos/$(BF_ASSETS_GITHUB_REPO)/releases/assets/$(call get_fw_asset_id_gh, $(1))"
	$(1)_CURL_OPTIONS=-H "Accept: application/octet-stream" -H "Authorization: token $(SDK_ASSETS_GITHUB_TOKEN)"
	$(1)_SKIP_VERSION=y

endef

define make_url_bfsoc
	$(1)_URL="https://api.github.com/repos/$(BF_ASSETS_GITHUB_REPO)/releases/assets/$(call get_bfsoc_asset_id_gh, $(1))"
	$(1)_CURL_OPTIONS=-H "Accept: application/octet-stream" -H "Authorization: token $(SDK_ASSETS_GITHUB_TOKEN)"
	$(1)_SKIP_VERSION=y

endef



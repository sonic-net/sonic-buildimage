#
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES.
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

DPU_NIC_FW_BASE_URL =
DPU_NIC_FW_VERSION =
BF3_FW_BASE_URL =

BF3_FW_VERSION = 47.1080

BF3_FW_FILE = fw-BlueField-3-rel-32_$(subst .,_,$(BF3_FW_VERSION)).mfa

define make_url_fw
	$(1)_URL="https://github.com/Mellanox/sonic-bluefield-packages/releases/download/dpu-fw-$(BF3_FW_VERSION)/$(1)"

endef


ifeq ($(BF3_FW_BASE_URL),)
$(eval $(foreach fw,$(BF3_FW_FILE),$(call make_url_fw,$(fw))))
else
$(BF3_FW_FILE)_URL = $(BF3_FW_BASE_URL)/$(BF3_FW_FILE)
endif

BF_FW_FILES = $(BF3_FW_FILE)

export BF3_FW_VERSION
export BF3_FW_FILE
export BF_FW_FILES

SONIC_ONLINE_FILES += $(BF_FW_FILES)


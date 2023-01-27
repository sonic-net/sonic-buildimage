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
# mellanox asic firmware

MLNX_FW_BASE_PATH = $(MLNX_SDK_BASE_PATH)

# Place an URL here to FW if you want to download FW instead
MLNX_FW_BASE_URL =

SIMX_VERSION = 5.1-1065

ifneq ($(MLNX_FW_BASE_URL), )
FW_FROM_URL = y
else
FW_FROM_URL = n
endif

MLNX_SPC_FW_VERSION = 13.2010.4150
MLNX_SPC_FW_FILE = fw-SPC-rel-$(subst .,_,$(MLNX_SPC_FW_VERSION))-EVB.mfa
$(MLNX_SPC_FW_FILE)_PATH = $(MLNX_FW_BASE_PATH)
$(MLNX_SPC_FW_FILE)_URL = $(MLNX_FW_BASE_URL)/$(MLNX_SPC_FW_FILE)

MLNX_SPC2_FW_VERSION = 29.2010.4150
MLNX_SPC2_FW_FILE = fw-SPC2-rel-$(subst .,_,$(MLNX_SPC2_FW_VERSION))-EVB.mfa
$(MLNX_SPC2_FW_FILE)_PATH = $(MLNX_FW_BASE_PATH)
$(MLNX_SPC2_FW_FILE)_URL = $(MLNX_FW_BASE_URL)/$(MLNX_SPC2_FW_FILE)

MLNX_SPC3_FW_VERSION = 30.2010.4150
MLNX_SPC3_FW_FILE = fw-SPC3-rel-$(subst .,_,$(MLNX_SPC3_FW_VERSION))-EVB.mfa
$(MLNX_SPC3_FW_FILE)_PATH = $(MLNX_FW_BASE_PATH)
$(MLNX_SPC3_FW_FILE)_URL = $(MLNX_FW_BASE_URL)/$(MLNX_SPC3_FW_FILE)

MLNX_SPC4_FW_VERSION = 34.2010.4150
MLNX_SPC4_FW_FILE = fw-SPC4-rel-$(subst .,_,$(MLNX_SPC4_FW_VERSION))-EVB.mfa
$(MLNX_SPC4_FW_FILE)_PATH = $(MLNX_FW_BASE_PATH)
$(MLNX_SPC4_FW_FILE)_URL = $(MLNX_FW_BASE_URL)/$(MLNX_SPC4_FW_FILE)

MLNX_FW_FILES = $(MLNX_SPC_FW_FILE) $(MLNX_SPC2_FW_FILE) $(MLNX_SPC3_FW_FILE) $(MLNX_SPC4_FW_FILE)

ifeq ($(FW_FROM_URL),n)
SONIC_COPY_FILES += $(MLNX_FW_FILES)
else
SONIC_ONLINE_FILES += $(MLNX_FW_FILES)
endif

MLNX_FILES += $(MLNX_FW_FILES)

export MLNX_SPC_FW_FILE
export MLNX_SPC2_FW_FILE
export MLNX_SPC3_FW_FILE
export MLNX_SPC4_FW_FILE

#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2025-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

# sedutil package for Mellanox platforms
SEDUTIL_GITHUB_URL = https://github.com/ChubbyAnt/sedutil
SEDUTIL_VERSION = 1.15-5ad84d8
SEDUTIL = sedutil_$(SEDUTIL_VERSION)_$(CONFIGURED_ARCH).deb
$(SEDUTIL)_SRC_PATH = $(PLATFORM_PATH)/sedutil

SONIC_MAKE_DEBS += $(SEDUTIL)

export SEDUTIL_GITHUB_URL
export SEDUTIL_VERSION
export SEDUTIL

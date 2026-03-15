#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

# Mellanox Platform Utils package

MELLANOX_PLATFORM_UTILS = mellanox_platform_utils-1.0.0-py3-none-any.whl
$(MELLANOX_PLATFORM_UTILS)_SRC_PATH = $(PLATFORM_PATH)/platform-utils
$(MELLANOX_PLATFORM_UTILS)_DEPENDS = $(SONIC_PLATFORM_API_PY3)
$(MELLANOX_PLATFORM_UTILS)_DEBS_DEPENDS = $(PYTHON3_SWSSCOMMON)

$(MELLANOX_PLATFORM_UTILS)_PYTHON_VERSION = 3
SONIC_PYTHON_WHEELS += $(MELLANOX_PLATFORM_UTILS)

export mellanox_platform_utils_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(MELLANOX_PLATFORM_UTILS))"

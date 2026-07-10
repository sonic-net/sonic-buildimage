#
# Copyright (c) 2019-2021 NVIDIA CORPORATION & AFFILIATES.
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

# SONIC_PLATFORM_API_PY3 package

SONIC_PLATFORM_API_PY3 = mlnx_platform_api-1.0-py3-none-any.whl
$(SONIC_PLATFORM_API_PY3)_SRC_PATH = $(PLATFORM_PATH)/mlnx-platform-api
$(SONIC_PLATFORM_API_PY3)_PYTHON_VERSION = 3
$(SONIC_PLATFORM_API_PY3)_DEPENDS = $(SONIC_PY_COMMON_PY3) $(SONIC_PLATFORM_COMMON_PY3) $(SONIC_CONFIG_ENGINE_PY3) $(SONIC_PLATFORM_API_PY2)
$(SONIC_PLATFORM_API_PY3)_DEBS_DEPENDS = $(PYTHON3_SWSSCOMMON)
SONIC_PYTHON_WHEELS += $(SONIC_PLATFORM_API_PY3)

export mlnx_platform_api_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PLATFORM_API_PY3))"

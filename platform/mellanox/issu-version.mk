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
# ISSU version

ISSU_VERSION_FILE = issu-version
$(ISSU_VERSION_FILE)_SRC_PATH = $(PLATFORM_PATH)/issu-version

$(ISSU_VERSION_FILE)_DEPENDS += $(SYSSDK)

SONIC_MAKE_FILES += $(ISSU_VERSION_FILE)


export ISSU_VERSION_FILE

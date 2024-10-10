#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

# mellanox cpld firmware
MLNX_SN2700_CPLD_ARCHIVE = msn2700_cpld.tar.gz
$(MLNX_SN2700_CPLD_ARCHIVE)_PATH = platform/mellanox/cpld/
SONIC_COPY_FILES += $(MLNX_SN2700_CPLD_ARCHIVE)
MLNX_CPLD_ARCHIVES += $(MLNX_SN2700_CPLD_ARCHIVE)
export MLNX_CPLD_ARCHIVES

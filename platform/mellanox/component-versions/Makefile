#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = component-versions

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	./create_component_versions.sh $(MLNX_SDK_VERSION) $(MLNX_SPC_FW_VERSION) $(MLNX_SAI_VERSION) $(MLNX_HW_MANAGEMENT_VERSION) $(MFT_VERSION) $(MFT_REVISION) $(KVERSION_SHORT) $(SIMX_VERSION) $(MLNX_RSHIM_DRIVER_VERSION)
	mv temp_versions_file $(DEST)/$(MAIN_TARGET)

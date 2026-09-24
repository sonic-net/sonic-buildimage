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

.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = $(DOCA_COMMON)
DERIVED_TARGETS = $(DOCA_DEBS) $(DOCA_DEV_DEBS) $(DOCA_DBG_DEBS)
DOCA_PATH := $(shell pwd)/doca

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Make sure errors will be reported
	set -e

	# get sources
	rm -rf doca/ doca.zip

	# get sources
	wget $(SDK_SOURCE_URL)/doca.zip
	unzip doca.zip

	pushd doca/
	find . -type f -exec touch {} +

	source /etc/profile.d/mlnx-dpdk-aarch64-linux-gnu.sh
	source ./devtools/public/set_env_variables.sh  --deb

	python3 ./devtools/public/split_sdk_to_source_components.py

	pushd /tmp/doca_delivery
	sudo -E $(DOCA_PATH)/devtools/scripts/doca_build_packages.sh --dpu --deb $(DOCA_PATH)/../packages.txt
	popd

	sudo chown -R ${USER} /tmp/doca_sdk_packages/
	pushd /tmp/doca_sdk_packages
	mv $(DERIVED_TARGETS) $* $(DEST)/
	popd

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

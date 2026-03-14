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

.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = $(DPDK)
DERIVED_TARGETS = $(DPDK_DEV)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Make sure errors will be reported
	set -e

	rm -rf dpdk.org/ dpdk.org.zip

	# get sources
	wget $(SDK_SOURCE_URL)/dpdk.org.zip
	unzip dpdk.org.zip
	pushd dpdk.org/
	find . -type f -exec touch {} +

	patch -p1 < ../0001-Remove-meson-from-dependencies.patch

	# Build the package
	# PATH variable is required for ninja to find the right version
	DPDK_CONFIG_OPTIONS='--cross-file ../config/arm/arm64_bluefield_linux_gcc' DEB_BUILD_OPTIONS='dpdk_config_options=-Dmachine=0xd08' dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)

	popd
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

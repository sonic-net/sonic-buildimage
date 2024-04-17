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

MAIN_TARGET = $(FLEXIO)
DERIVED_TARGETS =

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Make sure errors will be reported
	set -e

	# get sources
	rm -rf flexio-sdk/ flexio-sdk.zip

	# get sources
	wget $(SDK_SOURCE_URL)/flexio-sdk.zip
	unzip flexio-sdk.zip

	pushd flexio-sdk/
	find . -type f -exec touch {} +

	patch -p1 < ../0001-Allow-warning-during-package-compilation.patch
	patch -p1 < ../0001-Install-LD-configuration.patch
	patch -p1 < ../0001-Remove-meson-from-dependencies.patch

	# Build the Debs
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)

	popd
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

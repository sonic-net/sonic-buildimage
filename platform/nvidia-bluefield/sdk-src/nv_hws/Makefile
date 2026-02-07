#
# Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES.
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

MAIN_TARGET = $(LIB_NV_HWS)
DERIVED_TARGETS = $(LIB_NV_HWS_DEV)
NV_HWS_PATH := $(shell pwd)/nv_hws

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Make sure errors will be reported
	set -e

	# get sources
	rm -rf nv_hws/ nv_hws.zip

	# get sources
	wget $(SDK_SOURCE_URL)/nv_hws.zip
	unzip nv_hws.zip

	pushd nv_hws/

	find . -type f -exec touch {} +

	PATH=/usr/bin/:$(PATH) dpkg-buildpackage -d -us -uc -b -j1 --admindir $(SONIC_DPKG_ADMINDIR)

	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

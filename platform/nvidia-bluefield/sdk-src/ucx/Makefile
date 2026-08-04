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

MAIN_TARGET = $(UCX)

OFED_TAR = MLNX_OFED_SRC-debian-$(OFED_VER_FULL).tgz
OFED_SRC = MLNX_OFED_SRC-$(OFED_VER_FULL)
UCX_TAR = ucx_*.orig.tar.gz

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Make sure errors will be reported
	set -e

	sudo rm -rf MLNX_OFED_SRC*
	wget $(SDK_SOURCE_URL)/$(OFED_TAR)
	tar xf $(OFED_TAR)
	pushd $(OFED_SRC)/SOURCES
	tar xf $(UCX_TAR)
	pushd ucx-*/
	find . -type f -exec touch {} +

	patch -p0 < ../../../disable-libucx0-conflict.patch
	patch -p1 < ../../../disable-go-bindings-generation.patch

	# Build the Debs
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)

	popd
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

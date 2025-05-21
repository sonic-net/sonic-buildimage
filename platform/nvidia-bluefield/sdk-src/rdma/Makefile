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

OFED_TAR = MLNX_OFED_SRC-debian-$(OFED_VER_FULL).tgz
OFED_SRC = MLNX_OFED_SRC-$(OFED_VER_FULL)
RDMA_TAR = rdma-core*.orig.tar.gz

MAIN_TARGET = $(RDMA_CORE)
DERIVED_TARGETS = $(RDMA_CORE_DERIVED_DEBS)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Make sure errors will be reported
	set -e

	sudo rm -rf MLNX_OFED_SRC*
	wget $(SDK_SOURCE_URL)/$(OFED_TAR)
	tar xf $(OFED_TAR)
	pushd $(OFED_SRC)/SOURCES
	tar xf $(RDMA_TAR)
	pushd rdma-core-*/
	find . -type f -exec touch {} +

	# Comment out the Breaks dependency on ibverbs-providers to avoid circular dependency issues
	# during package installation, since ibverbs-providers and libibverbs1 depend on each other
	sed -i 's/^Breaks: ibverbs-providers/#Breaks: ibverbs-providers/' ./debian/control

	find . -type f -exec touch {} +

	# Build the Debs
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)

	popd
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

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

MAIN_TARGET = $(DOCA_LIBS)
DERIVED_TARGETS = $(DOCA_LIBS_DEV) $(DOCA_LIBS_DBG)

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

	patch -p1 < ../0001-Allow-compilation-with-warnings.patch
	patch -p1 < ../0001-Remove-meson-from-connectx-dependencies.patch

	source /etc/profile.d/libgrpc.sh
	source /etc/profile.d/mlnx-dpdk-aarch64-linux-gnu.sh

	# Build the Debs
	PATH=/opt/mellanox/grpc/bin:/usr/bin/:$(PATH) ./devtools/doca_package_build.sh connectx

	popd
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

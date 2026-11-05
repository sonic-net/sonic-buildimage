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
.SHELLFLAGS += -ex

MLXBF_BOOTCTL = mlxbf-bootctl-${MLXBF_BOOTCTL_DEB_VERSION}
GET_PACKAGE := $(shell pwd)/../recipes/get-soc-package.sh

$(addprefix $(DEST)/, $(MLXBF_BOOTCTL_DEB)): $(DEST)/% :
	rm -rf ./tmp/
	mkdir ./tmp/
	pushd ./tmp/

	${GET_PACKAGE} ${BSD_BASE_SOURCE_URL} ${MLXBF_BOOTCTL}

	pushd ${MLXBF_BOOTCTL}

	# Build the package
	DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -rfakeroot -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)

	popd

	mv $* $(DEST)/

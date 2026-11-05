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

$(addprefix $(DEST)/, $(MLNX_BLUEFIELD_BUILD_SCRIPTS)): $(DEST)/% :
	rm -rf ./tmp/
	mkdir ./tmp/
	pushd ./tmp/

	wget -O ${MLNX_BLUEFIELD_BUILD_SCRIPTS} "${BSD_BASE_URL}/install/distro/DEBS/${MLNX_BLUEFIELD_BUILD_SCRIPTS}"

	mkdir -p bfscripts/DEBIAN
	dpkg -e ${MLNX_BLUEFIELD_BUILD_SCRIPTS} bfscripts/DEBIAN
	dpkg -x ${MLNX_BLUEFIELD_BUILD_SCRIPTS} bfscripts
	rm -f ${MLNX_BLUEFIELD_BUILD_SCRIPTS}

	# Remove postinst script as we don't want to enable systemd services from the package. 
	# We are interested only in a couple of scripts that package has.
	rm -f bfscripts/DEBIAN/postinst

	dpkg -b bfscripts ./

	mv ${MLNX_BLUEFIELD_BUILD_SCRIPTS} $(DEST)/
	popd

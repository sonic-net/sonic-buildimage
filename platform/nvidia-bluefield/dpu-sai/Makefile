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

MAIN_TARGET = $(DPU_SAI)
DERIVED_TARGETS = $(DPU_SAI_DBGSYM)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :

	rm -rf mlnx_sai_src
	wget -c $(DPU_SAI_SOURCE_BASE_URL)/$(DPU_SAI_VERSION).tar.gz -O - | tar -xz --one-top-level=mlnx_sai_src
	pushd mlnx_sai_src
	pushd mlnx_sai

	# Build the package 
	debuild -e 'make_extra_flags="DEFS=-DACS_OS -DCONFIG_SYSLOG"' -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)

	popd

	mv $* $(DERIVED_TARGETS) $(DEST)/
	popd

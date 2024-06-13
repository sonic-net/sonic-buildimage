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
.SHELLFLAGS += -e

$(addprefix $(DEST)/, $(BF_PLATFORM_MODULE)): $(DEST)/% :

	# Clean up
	rm -rf drivers/* \
		   cfg/bluefield-modules.conf \
		   debian/bluefield-platform-modules.postinst

	mkdir -p drivers/
	mkdir -p cfg/

	$(foreach kernel_module, $(BF_PLATFORM_DRIVERS), \
		cp $(PROJECT_ROOT)/$(FILES_PATH)/$(kernel_module) drivers/
	)

	export driver_targets="$(BF_PLATFORM_DRIVERS)"
	export kversion="$(KVERSION)"
	j2 templates/bluefield-platform-modules.postinst.j2 > debian/bluefield-platform-modules.postinst
	j2 templates/bluefield-modules.conf.j2 > cfg/bluefield-modules.conf

	chmod +x debian/bluefield-platform-modules.postinst
	chmod 0644 cfg/bluefield-modules.conf

	DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -rfakeroot -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS)

	mv ../$(BF_PLATFORM_MODULE) $(DEST)/

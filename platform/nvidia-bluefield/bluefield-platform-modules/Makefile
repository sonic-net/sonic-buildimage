#
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES.
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
DKMS_ROOT = /var/lib/dkms/

$(addprefix $(DEST)/, $(BF_PLATFORM_MODULE)): $(DEST)/% :

	# Clean up
	rm -rf drivers/* \
		   cfg/bluefield-modules.conf \
		   debian/bluefield-platform-modules.postinst

	mkdir -p drivers/
	mkdir -p cfg/

	# Note: Drivers are installed in DKMS_ROOT when the corresponding DKMS package is installed, no need to build again
	$(foreach kernel_module, $(BF_PLATFORM_DRIVERS), \
		cp $(DKMS_ROOT)/$(kernel_module)/kernel-$(KVERSION)-$(CONFIGURED_ARCH)/module/$(kernel_module).ko drivers/
	)

	# Driver name for mlx-bootctl is different from the name in the recipe, so add a explicit rule
	cp $(DKMS_ROOT)/mlx-bootctl/kernel-$(KVERSION)-$(CONFIGURED_ARCH)/module/mlxbf-bootctl.ko drivers/

	export driver_targets="$(BF_PLATFORM_DRIVERS)"
	export kversion="$(KVERSION)"
	j2 templates/bluefield-platform-modules.postinst.j2 > debian/bluefield-platform-modules.postinst
	j2 templates/bluefield-modules.conf.j2 > cfg/bluefield-modules.conf

	chmod +x debian/bluefield-platform-modules.postinst
	chmod 0644 cfg/bluefield-modules.conf

	DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -rfakeroot -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS)

	mv ../$(BF_PLATFORM_MODULE) $(DEST)/

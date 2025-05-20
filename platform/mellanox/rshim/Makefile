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

SRPMS_URL = $(MLNX_BSD_RSHIM_SRC_BASE_URL)/install/distro/SRPMS/
RSHIM_URL = $(shell lynx --listonly --nonumbers -dump $(SRPMS_URL) | grep rshim)

$(addprefix $(DEST)/, $(MLNX_RSHIM)): $(DEST)/% :
	rm -rf ./rshim*
	mkdir ./rshim_src

	wget ${RSHIM_URL}
	rpm2cpio rshim-${MLNX_RSHIM_DRIVER_VERSION}*.src.rpm | cpio -idmv
	tar xf rshim-${MLNX_RSHIM_DRIVER_VERSION}.tar.gz -C rshim_src --strip-components=1

	pushd rshim_src

	# disable default systemd service
	sed -i 's/	dh_auto_configure/	dh_auto_configure -- --with-systemdsystemunitdir=no/' debian/rules

	# Build the package
	DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -rfakeroot -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)

	popd

	mv $* $(DEST)/

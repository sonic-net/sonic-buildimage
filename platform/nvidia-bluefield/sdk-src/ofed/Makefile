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
BUILD_ARCH := $(shell dpkg-architecture -qDEB_BUILD_ARCH)

OFED_TAR = MLNX_OFED_SRC-debian-$(OFED_VER_FULL).tgz
OFED_SRC = MLNX_OFED_SRC-$(OFED_VER_FULL)

DKMS_CTRL = /var/lib/dkms/mlnx-ofed-kernel/
DKMS_TMP := $(shell mktemp -u -d -t dkms.XXXXXXXXXX)
UNAME_TMP := $(shell mktemp -u -d -t uname.XXXXXXXXXX)

MLNX_TOOLS_TMP = /tmp/mlnx_tools_build/

$(addprefix $(DEST)/, $(MLNX_TOOLS)): $(DEST)/% :
	# Make sure errors will be reported
	set -e
	mkdir -p $(MLNX_TOOLS_TMP) && pushd $(MLNX_TOOLS_TMP)
	sudo rm -rf MLNX_OFED_SRC*
	wget $(SDK_SOURCE_URL)/$(OFED_TAR)
	tar xf $(OFED_TAR)

	pushd $(OFED_SRC)/
	find . -type f -exec touch {} +

	# Build the Debs
	sudo ./install.pl --kernel-only --build-only --without-depcheck --force  \
						--with-mlnx-tools --without-knem --without-knem-dkms \
						--without-kernel-mft-dkms

	sudo chown -R ${USER} ./*

	pushd DEBS/*/aarch64/
	chown ${USER} $(MLNX_TOOLS) $(OFED_KERNEL_UTILS) $(OFED_KERNEL_DKMS) 
	mv $(MLNX_TOOLS) $(OFED_KERNEL_UTILS) $(OFED_KERNEL_DKMS) $(DEST)/
	popd

	popd
	popd
	rm -rf $(MLNX_TOOLS_TMP)

$(addprefix $(DEST)/, $(OFED_KERNEL_UTILS)): $(DEST)/% : $(DEST)/$(MLNX_TOOLS)
$(addprefix $(DEST)/, $(OFED_KERNEL_DKMS)): $(DEST)/% : $(DEST)/$(MLNX_TOOLS)

$(addprefix $(DEST)/, $(OFED_KERNEL)): $(DEST)/% :
	# Make sure errors will be reported
	set -e

	# Prepare the directory to build mlnx-ofed-kernel-modules debian package
	mkdir -p $(DKMS_TMP)/DEBIAN
	mkdir -p $(DKMS_TMP)/lib/modules/$(KVERSION)/updates/dkms/
	export kversion="$(KVERSION)"
	export mft_version="$(OFED_KERNEL_VER_FULL)"
	j2 templates/control.j2 > $(DKMS_TMP)/DEBIAN/control
	j2 templates/postinst.j2 > $(DKMS_TMP)/DEBIAN/postinst
	j2 templates/postrm.j2 > $(DKMS_TMP)/DEBIAN/postrm
	chmod +x $(DKMS_TMP)/DEBIAN/postinst
	chmod +x $(DKMS_TMP)/DEBIAN/postrm

	# Note: Drivers are installed in DKMS_ROOT when the corresponding DKMS package is installed, no need to build again
	cp -r $(DKMS_CTRL)/kernel-$(KVERSION)-aarch64/module/*.ko \
			$(DKMS_TMP)/lib/modules/$(KVERSION)/updates/dkms/

	dpkg -b $(DKMS_TMP) .

	chown ${USER} $(OFED_KERNEL)
	mv $(OFED_KERNEL) $(DEST)/

	rm -rf $(DKMS_TMP)

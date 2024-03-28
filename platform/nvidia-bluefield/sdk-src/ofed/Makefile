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
BUILD_ARCH = $(shell dpkg-architecture -qDEB_BUILD_ARCH)

MAIN_TARGET = $(OFED_KERNEL)

OFED_TAR = MLNX_OFED_SRC-debian-$(OFED_VER_FULL).tgz
OFED_SRC = MLNX_OFED_SRC-$(OFED_VER_FULL)

SRC_DEB = mlnx-ofed-kernel-dkms_$(OFED_VER_SHORT)*_all.deb
SRC_DEB_DEPENDENCIES = mlnx-tools_*_$(BUILD_ARCH).deb \
		mlnx-ofed-kernel-utils_$(OFED_VER_SHORT)*_$(BUILD_ARCH).deb

DKMS_CTRL = /var/lib/dkms/mlnx-ofed-kernel/
DKMS_TMP := $(shell mktemp -u -d -t dkms.XXXXXXXXXX)

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
						--without-kernel-mft-dkms --without-mlnx-ofed-kernel-dkms

	sudo chown -R ${USER} ./*

	pushd DEBS/*/aarch64/
	chown ${USER} $(MLNX_TOOLS) $(OFED_KERNEL_UTILS)
	mv $(MLNX_TOOLS) $(OFED_KERNEL_UTILS) $(DEST)/
	popd

	popd
	popd
	rm -rf $(MLNX_TOOLS_TMP)

$(addprefix $(DEST)/, $(OFED_KERNEL_UTILS)): $(DEST)/% : $(DEST)/$(MLNX_TOOLS)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
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

	sudo rm -rf MLNX_OFED_SRC*
	wget $(SDK_SOURCE_URL)/$(OFED_TAR)
	tar xf $(OFED_TAR)

	pushd $(OFED_SRC)/
	find . -type f -exec touch {} +

	sudo mv /bin/uname /bin/uname.orig
	cat << EOF >> /tmp/uname
	#!/bin/bash
	if [[ "\$$1" == "-r" ]]; then
		echo $(KVERSION)
		exit 0
	fi
	/bin/uname.orig \$$@
	EOF
	chmod +x /tmp/uname
	sudo mv /tmp/uname /bin/uname

	# Build the Debs
	sudo ./install.pl -k $(KVERSION) --kernel-only --build-only --without-depcheck --kernel-extra-args --force --with-mlnx-tools

	sudo chown -R ${USER} ./*

	pushd DEBS/*/aarch64/

	# put a lock here because dpkg does not allow installing packages in parallel
	while true; do
		if mkdir $(DEST)/dpkg_lock &> /dev/null; then
			{ sudo dpkg -i $(SRC_DEB_DEPENDENCIES) $(SRC_DEB) && rm -d $(DEST)/dpkg_lock && break; } || { rm -d $(DEST)/dpkg_lock && exit 1 ; }
		fi
	done

	sudo mv /bin/uname.orig /bin/uname
	sudo dkms build mlnx-ofed-kernel/$(OFED_KERNEL_VER_FULL) -k $(KVERSION) -a $(BUILD_ARCH)

	# copy the built modules
	cp -r $(DKMS_CTRL)/kernel-$(KVERSION)-aarch64/module/*.ko \
			$(DKMS_TMP)/lib/modules/$(KVERSION)/updates/dkms/

	dpkg -b $(DKMS_TMP) .

	chown ${USER} $(MAIN_TARGET)
	mv $(MAIN_TARGET) $(DEST)/

	rm -rf $(DKMS_TMP)

	popd
	popd

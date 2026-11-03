#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2016-2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

ifeq ($(CONFIGURED_ARCH), amd64)
MFT_ARCH = x86_64
else
MFT_ARCH = $(CONFIGURED_ARCH)
endif

MFT_NAME = mft-$(MFT_VERSION)-$(MFT_REVISION)-$(MFT_ARCH)-deb
MFT_TGZ = $(MFT_NAME).tgz

ifeq ($(MFT_FROM_INTERNAL),y)
MFT_TGZ_URL = $(MLNX_MFT_INTERNAL_SOURCE_BASE_URL)$(MFT_TGZ)
else
MFT_TGZ_URL = http://www.mellanox.com/downloads/MFT/$(MFT_TGZ)
endif

SRC_DEB = kernel-mft-dkms_$(MFT_VERSION)-$(MFT_REVISION)_all.deb
KERNEL_MFT_TARGET = kernel-mft-dkms-modules-$(KVERSION)_$(MFT_VERSION)_$(CONFIGURED_ARCH).deb

MAIN_TARGET = mft_$(MFT_VERSION)-$(MFT_REVISION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = mft-oem_$(MFT_VERSION)-$(MFT_REVISION)_$(CONFIGURED_ARCH).deb

DKMS_CTRL = /var/lib/dkms/kernel-mft-dkms/
DKMS_TMP := $(shell mktemp -u -d -t dkms.XXXXXXXXXX)

MFT_TMP = /tmp/mft_build/

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	mkdir -p $(MFT_TMP)
	pushd $(MFT_TMP)
	wget -O $(MFT_TGZ) $(MFT_TGZ_URL)
	tar xzf $(MFT_TGZ)

	# fix timestamp
	touch $(MFT_NAME)/DEBS/*.deb
	mv $(MFT_NAME)/DEBS/*.deb $(DEST)
	popd
	rm -rf $(MFT_TMP)

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

$(addprefix $(DEST)/, $(KERNEL_MFT_TARGET)): $(DEST)/% :
	rm -rf $(MFT_NAME)
	wget -O $(MFT_TGZ) $(MFT_TGZ_URL)
	tar xzf $(MFT_TGZ)

	# Prepare the directory to build kernel-mft-dkms-modules debian package
	mkdir -p $(DKMS_TMP)/DEBIAN
	mkdir -p $(DKMS_TMP)/lib/modules/$(KVERSION)/updates/dkms/
	export kversion="$(KVERSION)"
	export mft_version="$(MFT_VERSION)"
	j2 templates/control.j2 > $(DKMS_TMP)/DEBIAN/control
	j2 templates/postinst.j2 > $(DKMS_TMP)/DEBIAN/postinst
	j2 templates/postrm.j2 > $(DKMS_TMP)/DEBIAN/postrm
	chmod +x $(DKMS_TMP)/DEBIAN/postinst
	chmod +x $(DKMS_TMP)/DEBIAN/postrm

	pushd $(MFT_NAME)/SDEBS

	# put a lock here because dpkg does not allow installing packages in parallel.
	# Starting MFT 4.37 the package postinst builds the module for the kernel
	# given in the kversion env var (falling back to the build host kernel);
	# pass it through sudo so it targets the SONiC kernel.
	flock $(DEST)/dpkg_lock.lk sudo kversion=$(KVERSION) dpkg -i $(SRC_DEB)

	popd

	if [ -d /usr/src/kernel-mft-dkms-$(MFT_VERSION).$(MFT_REVISION) ]; then
		DKMS_VER=$(MFT_VERSION).$(MFT_REVISION)
	else
		DKMS_VER=$(MFT_VERSION)
	fi
	sudo dkms build kernel-mft-dkms/$${DKMS_VER} -k $(KVERSION) -a $(CONFIGURED_ARCH)

	# copy the built modules. Take them from the dkms build tree rather than the
	# kernel-<kver>-<arch> symlink: that symlink is only created by "dkms install",
	# which MFT 4.37+ postinst runs for the uname -m arch (x86_64), not amd64.
	cp -r $(DKMS_CTRL)/$${DKMS_VER}/$(KVERSION)/$(CONFIGURED_ARCH)/module/*.ko \
			$(DKMS_TMP)/lib/modules/$(KVERSION)/updates/dkms/

	pushd $(MFT_NAME)/DEBS
	dpkg -b $(DKMS_TMP) .
	popd

	rm -rf $(DKMS_TMP)

	mv $(MFT_NAME)/DEBS/$(KERNEL_MFT_TARGET) $(DEST)



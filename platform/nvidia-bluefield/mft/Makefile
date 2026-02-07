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

MFT_ARCH = aarch64
MFT_NAME = mft-$(MFT_VERSION)-$(MFT_REVISION)-$(MFT_ARCH)-deb
MFT_TGZ = $(MFT_NAME).tgz

ifeq ($(MFT_FROM_INTERNAL),y)
MFT_TGZ_URL = $(MFT_INTERNAL_SOURCE_BASE_URL)/$(MFT_TGZ)
else
MFT_TGZ_URL = http://www.mellanox.com/downloads/MFT/$(MFT_TGZ)
endif

BUILD_ARCH := $(shell dpkg-architecture -qDEB_BUILD_ARCH)
KERNEL_MFT_TARGET = kernel-mft-dkms-modules-$(KVERSION)_$(MFT_VERSION)_$(BUILD_ARCH).deb

MAIN_TARGET = mft_$(MFT_VERSION)-$(MFT_REVISION)_arm64.deb
DERIVED_TARGETS = mft-oem_$(MFT_VERSION)-$(MFT_REVISION)_arm64.deb

DKMS_CTRL = /var/lib/dkms/kernel-mft-dkms/
DKMS_TMP := $(shell mktemp -u -d -t dkms.XXXXXXXXXX)

MFT_TMP = /tmp/mft_build/

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	mkdir -p $(MFT_TMP)
	pushd $(MFT_TMP)
	rm -rf $(MFT_NAME)
	wget -O $(MFT_TGZ) $(MFT_TGZ_URL)
	tar xzf $(MFT_TGZ)

	# fix timestamp because we do not actually build tools, only kernel
	touch $(MFT_NAME)/DEBS/*.deb
	touch $(MFT_NAME)/SDEBS/*.deb
	mv $(MFT_NAME)/DEBS/*.deb $(DEST)
	mv $(MFT_NAME)/SDEBS/*.deb $(DEST)
	popd
	rm -rf $(MFT_TMP)

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

$(addprefix $(DEST)/, $(KERNEL_MFT_TARGET)): $(DEST)/% :
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

	# Note: Drivers are installed in DKMS_ROOT when the corresponding DKMS package is installed, no need to build again
	cp -r $(DKMS_CTRL)/kernel-$(KVERSION)*/module/*.ko \
			$(DKMS_TMP)/lib/modules/$(KVERSION)/updates/dkms/

	dpkg -b $(DKMS_TMP) .

	rm -rf $(DKMS_TMP)

	mv $(KERNEL_MFT_TARGET) $(DEST)

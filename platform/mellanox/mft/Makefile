#
# Copyright (c) 2016-2021 NVIDIA CORPORATION & AFFILIATES.
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
MOD_DEB = kernel-mft-dkms-modules-$(KVERSION)_$(MFT_VERSION)_$(CONFIGURED_ARCH).deb

MAIN_TARGET = mft_$(MFT_VERSION)-$(MFT_REVISION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = $(MOD_DEB) mft-oem_$(MFT_VERSION)-$(MFT_REVISION)_$(CONFIGURED_ARCH).deb

DKMS_BMDEB = /var/lib/dkms/kernel-mft-dkms/$(MFT_VERSION)/bmdeb
DKMS_TMP := $(shell mktemp -u -d -t dkms.XXXXXXXXXX)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf $(MFT_NAME)
	wget -O $(MFT_TGZ) $(MFT_TGZ_URL)
	tar xzf $(MFT_TGZ)

	pushd $(MFT_NAME)/SDEBS

	# put a lock here because dpkg does not allow installing packages in parallel
	while true; do
	if mkdir $(DEST)/dpkg_lock &> /dev/null; then
	{ sudo dpkg -i $(SRC_DEB) && rm -d $(DEST)/dpkg_lock && break; } || { rm -d $(DEST)/dpkg_lock && exit 1 ; }
	fi
	done

	popd

	sudo dkms build kernel-mft-dkms/$(MFT_VERSION) -k $(KVERSION) -a $(CONFIGURED_ARCH)
	sudo dkms mkbmdeb kernel-mft-dkms/$(MFT_VERSION) -k $(KVERSION) -a $(CONFIGURED_ARCH)

	# w/a: remove dependencies
	mkdir -p $(DKMS_TMP)/DEBIAN

	dpkg -e $(DKMS_BMDEB)/$(MOD_DEB) $(DKMS_TMP)/DEBIAN
	dpkg -x $(DKMS_BMDEB)/$(MOD_DEB) $(DKMS_TMP)

	sed -i '/^Depends:/c\Depends:' $(DKMS_TMP)/DEBIAN/control

	pushd $(MFT_NAME)/DEBS
	dpkg -b $(DKMS_TMP) .
	popd

	rm -rf $(DKMS_TMP)

	# fix timestamp because we do not actually build tools, only kernel
	touch $(MFT_NAME)/DEBS/*.deb
	mv $(MFT_NAME)/DEBS/*.deb $(DEST)

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

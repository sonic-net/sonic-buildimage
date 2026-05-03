.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = sx-kernel_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb

PACKAGE_NAME = sys_sdk

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
        # get sources
	rm -rf $(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION)

	wget -c $(MLNX_SDK_SOURCE_BASE_URL)/$(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION).tar.gz -O - | tar -xz

        # build kernel-only package for host installation
	rm -rf builds/cmake

	cmake -B builds/cmake -DCMAKE_SYSTEM_VERSION=$(shell ls -t /lib/modules |head -1) -DSKIP_DEPMOD_IN_INSTALL_PHASE=on -DKERNEL_ONLY=ON -DCMAKE_PROJECT_VERSION=$(MLNX_SDK_DEB_VERSION)

	cmake --build builds/cmake -j$(SONIC_CONFIG_MAKE_JOBS)

	cd builds/cmake

	cpack -G DEB

	mv sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb $*

	mv $* $(DEST)/

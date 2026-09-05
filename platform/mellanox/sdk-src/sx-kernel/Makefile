.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = sx-kernel_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb

PACKAGE_NAME = sys_sdk

MLNX_SX_KERNEL_GITHUB_URL_BASE = $(MLNX_SDK_DRIVERS_GITHUB_URL)/archive/refs/heads

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
        # get sources

	if [ ! -z "$(MLNX_SDK_SOURCE_BASE_URL)" ]; then
		rm -rf $(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION)
		wget -c $(MLNX_SDK_SOURCE_BASE_URL)/$(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION).tar.gz -O - | tar -xz
		pushd $(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION)
	else
		rm -rf Spectrum-SDK-Drivers-$(MLNX_SDK_VERSION)
		wget -c $(MLNX_SX_KERNEL_GITHUB_URL_BASE)/$(MLNX_SDK_VERSION).zip
		unzip $(MLNX_SDK_VERSION).zip
		rm -rf $(MLNX_SDK_VERSION).zip

		pushd Spectrum-SDK-Drivers-$(MLNX_SDK_VERSION)
	fi

    # build kernel-only package for host installation
	rm -rf builds/cmake

	cmake -B builds/cmake -DCMAKE_SYSTEM_VERSION=$(shell ls -t /lib/modules | head -1) -DSKIP_DEPMOD_IN_INSTALL_PHASE=on -DSKIP_UNINSTALL_INSTALL_DEPENDENCY=on -DKERNEL_ONLY=ON

	cmake --build builds/cmake -j$(SONIC_CONFIG_MAKE_JOBS)

	cd builds/cmake

	cpack -G DEB

	mv sx-kernel_*_$(CONFIGURED_ARCH).deb $(DEST)/$*

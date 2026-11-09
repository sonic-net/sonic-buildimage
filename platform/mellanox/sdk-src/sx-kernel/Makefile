.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = sx-kernel_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = sx-kernel-dev_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
PACKAGE_NAME = sx_kernel

MLNX_SX_KERNEL_GITHUB_URL_BASE = $(MLNX_SDK_DRIVERS_GITHUB_URL)/archive/refs/heads

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# get sources

	if [ ! -z "$(MLNX_SDK_SOURCE_BASE_URL)" ]; then
		rm -rf sx_kernel-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION)
		wget -c $(MLNX_SDK_SOURCE_BASE_URL)/$(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION).tar.gz -O - | tar -xz
		pushd sx_kernel-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION)
	else
		rm -rf Spectrum-SDK-Drivers-$(MLNX_SDK_VERSION)
		wget -c $(MLNX_SX_KERNEL_GITHUB_URL_BASE)/$(MLNX_SDK_VERSION).zip
		unzip $(MLNX_SDK_VERSION).zip
		rm -rf $(MLNX_SDK_VERSION).zip

		pushd Spectrum-SDK-Drivers-$(MLNX_SDK_VERSION)
		ln -s ./sx_scripts/Makefile ./Makefile
		ln -s ./sx_scripts/makefile ./makefile
		ln -s ./sx_scripts/configure ./configure
	fi

	# build

	debuild -e KVERSION=$(KVERSION) -e KSRC_EXT=/lib/modules/$(KVERSION)/source/ -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS)

	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

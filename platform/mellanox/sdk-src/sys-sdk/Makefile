.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH)-dev.deb sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH)-dbgsym.ddeb

PACKAGE_NAME = sys_sdk

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
        # get sources
	rm -rf $(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION)
	
	@echo  'wget -c $(MLNX_SDK_SOURCE_BASE_URL)/$(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION).tar.gz'
	
	wget -c $(MLNX_SDK_SOURCE_BASE_URL)/$(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION).tar.gz -O - | tar -xz

        # build
	rm -rf builds/cmake

	cmake -B builds/cmake -DCMAKE_SYSTEM_VERSION=$(shell ls -t /lib/modules |head -1) -DPYTHON_INTERPRETERS=python3 -DSKIP_UNINSTALL_INSTALL_DEPENDENCY=on -DSKIP_DEPMOD_IN_INSTALL_PHASE=on -DUSE_API_TESTER=OFF -DUSE_KERNEL=OFF -DCMAKE_PROJECT_VERSION=$(MLNX_SDK_DEB_VERSION)
	
	cmake --build builds/cmake -j$(SONIC_CONFIG_MAKE_JOBS)
	
	cd builds/cmake
	
	cpack -G DEB

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

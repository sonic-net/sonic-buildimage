.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = hw-management_1.mlnx.$(MLNX_HW_MANAGEMENT_VERSION)_$(CONFIGURED_ARCH).deb
BMC_ONLY_TARGET = hw-management-bmc_1.mlnx.$(MLNX_HW_MANAGEMENT_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :

	pushd hw-mgmt
	git stash
	git apply -3 ../*.patch || exit 1
	chmod +x ./debian/rules
	DEB_BUILD_PROFILES=pkg.hw-mgmt.cpu-only KVERSION=$(KVERSION) LM_DEPENDS=0 dpkg-buildpackage -us -uc -b -rfakeroot -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd

	mv hw-management_*.deb $(DEST)/$*

$(addprefix $(DEST)/, $(BMC_ONLY_TARGET)): $(DEST)/% :

	pushd hw-mgmt
	chmod +x ./debian/rules
	DEB_BUILD_PROFILES=pkg.hw-mgmt.bmc-only KVERSION=$(KVERSION) LM_DEPENDS=0 dpkg-buildpackage -us -uc -b -rfakeroot -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd

	mv hw-management-bmc_*.deb $(DEST)/$*

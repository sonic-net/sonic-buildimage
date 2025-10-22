SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -xe

P4LANG_TARGET = p4lang-pi_$(P4LANG_PI_VERSION_FULL)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(P4LANG_TARGET)): $(DEST)/% :
# 	# Remove any stale files
	rm -rf p4lang-pi-$(P4LANG_PI_VERSION)
	dget -u p4lang-pi_$(P4LANG_PI_VERSION_FULL).dsc http://download.opensuse.org/repositories/home:/p4lang/Debian_11/p4lang-pi_$(P4LANG_PI_VERSION_FULL).dsc
	pushd p4lang-pi-$(P4LANG_PI_VERSION)
	patch -p1 -i ../p4lang-pi.patch/use-boost-1.83.patch
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -us -uc -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd
	mv $* $(DEST)/

P4LANG_BMV2_TARGET = p4lang-bmv2_$(P4LANG_BMV2_VERSION_FULL)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(P4LANG_BMV2_TARGET)): $(DEST)/% :
# 	# Remove any stale files
	rm -rf p4lang-bmv2-$(P4LANG_BMV2_VERSION)
	dget -u p4lang-bmv2_$(P4LANG_BMV2_VERSION_FULL).dsc http://download.opensuse.org/repositories/home:/p4lang/Debian_11/p4lang-bmv2_$(P4LANG_BMV2_VERSION_FULL).dsc
	pushd p4lang-bmv2-$(P4LANG_BMV2_VERSION)
	patch -p1 -i ../p4lang-bmv2.patch/use-boost-1.83.patch
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -us -uc -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd
	mv $* $(DEST)/

P4LANG_P4C_TARGET = p4lang-p4c_$(P4LANG_P4C_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(addprefix $(DEST)/, $(P4LANG_P4C_TARGET)): $(DEST)/% :
# 	# Remove any stale files
	rm -rf p4lang-p4c-$(P4LANG_P4C_VERSION)
	dget -u p4lang-p4c_$(P4LANG_P4C_VERSION_FULL).dsc http://download.opensuse.org/repositories/home:/p4lang/Debian_11/p4lang-p4c_$(P4LANG_P4C_VERSION_FULL).dsc
	pushd p4lang-p4c-$(P4LANG_P4C_VERSION)
	patch -p1 -i ../p4lang-p4c.patch/use-boost-1.83.patch
	patch -p1 -i ../p4lang-p4c.patch/fix-build-for-boost-1.83.patch
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -us -uc -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd
	mv $* $(DEST)/


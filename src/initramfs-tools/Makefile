.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = initramfs-tools_$(INITRAMFS_TOOLS_VERSION)_all.deb
DERIVED_TARGETS = initramfs-tools-core_$(INITRAMFS_TOOLS_VERSION)_all.deb

INITRAMFS_TOOLS_REVISION = v$(INITRAMFS_TOOLS_VERSION)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtaining the initramfs-tools
	rm -rf ./initramfs-tools
	git clone https://salsa.debian.org/kernel-team/initramfs-tools.git ./initramfs-tools

	# Patch
	pushd ./initramfs-tools
	git checkout $(INITRAMFS_TOOLS_REVISION)
	QUILT_PATCHES=.. quilt push -a

	# Build the package
	rm -f debian/*.debhelper.log
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = initramfs-tools_$(INITRAMFS_TOOLS_VERSION)_all.deb
DERIVED_TARGETS = initramfs-tools-core_$(INITRAMFS_TOOLS_VERSION)_all.deb

# Use a pinned tarball instead of git clone for reliability and speed.
# Tarball works here because quilt (used for patches) doesn't need git history.
# SHA256 ensures integrity and reproducibility.
INITRAMFS_TOOLS_TARBALL_URL = https://salsa.debian.org/kernel-team/initramfs-tools/-/archive/v$(INITRAMFS_TOOLS_VERSION)/initramfs-tools-v$(INITRAMFS_TOOLS_VERSION).tar.gz
INITRAMFS_TOOLS_TARBALL_SHA256 = 27a78cc25acc3ca3d9c78deca165bf001b09b260ce25a3f3756e47a0e7bc0554

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtaining the initramfs-tools from pinned tarball
	rm -rf ./initramfs-tools
	wget -q -O initramfs-tools.tar.gz "$(INITRAMFS_TOOLS_TARBALL_URL)"
	echo "$(INITRAMFS_TOOLS_TARBALL_SHA256)  initramfs-tools.tar.gz" | sha256sum -c -
	tar xzf initramfs-tools.tar.gz
	mv initramfs-tools-v$(INITRAMFS_TOOLS_VERSION) initramfs-tools
	rm -f initramfs-tools.tar.gz

	# Patch
	pushd ./initramfs-tools
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

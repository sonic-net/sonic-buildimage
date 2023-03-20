SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e

MAIN_TARGET = iproute2_$(IPROUTE2_VERSION_SONIC)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf iproute2-$(IPROUTE2_VERSION)

	wget -O iproute2_$(IPROUTE2_VERSION).orig.tar.xz http://deb.debian.org/debian/pool/main/i/iproute2/iproute2_$(IPROUTE2_VERSION).orig.tar.xz
	wget -O iproute2_$(IPROUTE2_VERSION_FULL).dsc http://deb.debian.org/debian/pool/main/i/iproute2/iproute2_$(IPROUTE2_VERSION_FULL).dsc
	wget -O iproute2_$(IPROUTE2_VERSION_FULL).debian.tar.xz http://deb.debian.org/debian/pool/main/i/iproute2/iproute2_$(IPROUTE2_VERSION_FULL).debian.tar.xz
	dpkg-source -x iproute2_$(IPROUTE2_VERSION_FULL).dsc

	pushd iproute2-$(IPROUTE2_VERSION)

	patch -p1 < ../patch/0001-patch-macsec-xpn-support.patch

ifeq ($(CROSS_BUILD_ENVIRON), y)
	patch -p1 < ../patch/cross-compile-changes.patch
	dpkg-buildpackage -us -uc -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $* $(DEST)/

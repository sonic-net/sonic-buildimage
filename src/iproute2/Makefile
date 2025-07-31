SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e

IPROUTE2_VERSION = 4.9.0
IPROUTE2_VERSION_FULL = $(IPROUTE2_VERSION)-1

MAIN_TARGET = iproute2_$(IPROUTE2_VERSION_FULL)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf iproute2-$(IPROUTE2_VERSION)

	wget -O iproute2_$(IPROUTE2_VERSION).orig.tar.xz -N "https://sonicstorage.blob.core.windows.net/public/iproute2_4.9.0.orig.tar.xz"
	wget -O iproute2_$(IPROUTE2_VERSION_FULL).dsc -N "https://sonicstorage.blob.core.windows.net/public/iproute2_4.9.0-1.dsc"
	wget -O iproute2_$(IPROUTE2_VERSION_FULL).debian.tar.xz -N "https://sonicstorage.blob.core.windows.net/public/iproute2_4.9.0-1.debian.tar.xz"
	dpkg-source -x iproute2_$(IPROUTE2_VERSION_FULL).dsc
	# Apply patch series of iproute2-4.9.0
	pushd iproute2-4.9.0
	patch -p0 tc/tc_core.c ../patch/001_tc.patch
	patch -p0 misc/arpd.c ../patch/002_arpd.patch
	patch -p0 lib/libnetlink.c ../patch/003_libnetlink.patch
	patch -p0 misc/cJSON.c ../patch/004_cJSON_c.patch
	patch -p0 misc/cJSON.h ../patch/005_cJSON_h.patch
	patch -p0 Makefile ../patch/006_Makefile.patch
	patch -p0 misc/Makefile ../patch/007_misc_Makefile.patch
	popd

	pushd iproute2-$(IPROUTE2_VERSION)
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd

	mv $* $(DEST)/

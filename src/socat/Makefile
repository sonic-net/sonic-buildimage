.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = socat_$(SOCAT_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./socat-1.7.4.1

	# Get source package
	wget -NO socat_$(SOCAT_VERSION).dsc "https://sonicstorage.blob.core.windows.net/public/debian/socat_1.7.4.1-3.dsc"
	wget -NO socat_$(SOCAT_VERSION).debian.tar.xz "https://sonicstorage.blob.core.windows.net/public/debian/socat_1.7.4.1-3.debian.tar.xz"
	wget -NO socat_1.7.4.1.orig.tar.gz "https://sonicstorage.blob.core.windows.net/public/debian/socat_1.7.4.1.orig.tar.gz"

	dpkg-source -x socat_$(SOCAT_VERSION).dsc

	# Build source and Debian packages
	pushd socat-1.7.4.1
	patch -p0 < ../patch/enable_readline.patch
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -rfakeroot -Pnocheck -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	# Move the newly-built .deb packages to the destination directory
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)


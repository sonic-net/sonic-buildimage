.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = socat_$(SOCAT_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./socat-1.7.4.1

	# Get source package from Debian mirror (Azure blob can return 403 in locked-down nets)
	wget -NO socat_$(SOCAT_VERSION).dsc "http://deb.debian.org/debian/pool/main/s/socat/socat_1.7.4.1-3.dsc"
	wget -NO socat_$(SOCAT_VERSION).debian.tar.xz "http://deb.debian.org/debian/pool/main/s/socat/socat_1.7.4.1-3.debian.tar.xz"
	wget -NO socat_1.7.4.1.orig.tar.gz "http://deb.debian.org/debian/pool/main/s/socat/socat_1.7.4.1.orig.tar.gz"

	dpkg-source -x socat_$(SOCAT_VERSION).dsc

	# Build source and Debian packages
	pushd socat-1.7.4.1
	patch -p0 < ../enable_readline.patch
	DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -rfakeroot -Pnocheck -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd

	# Move the newly-built .deb packages to the destination directory
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)


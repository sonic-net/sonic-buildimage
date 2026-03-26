.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(MAKEDUMPFILE)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf makedumpfile-$(MAKEDUMPFILE_VERSION)*

	# Get makedumpfile release
	dget https://deb.debian.org/debian/pool/main/m/makedumpfile/makedumpfile_$(MAKEDUMPFILE_VERSION)-$(MAKEDUMPFILE_DEB_REVISION).dsc
	pushd makedumpfile-$(MAKEDUMPFILE_VERSION)

	git init
	git add -f *
	git commit -m "unmodified makedumpfile source"

	# Build source and Debian packages
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	# Move the newly-built .deb package to the destination directory
	mv $* $(DEST)/

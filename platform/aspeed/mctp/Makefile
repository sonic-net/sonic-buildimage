# Build the mctp Debian package from the upstream CodeConstruct mctp release
# tarball (meson), using the debian/ metadata in this directory.

.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = $(MCTP)
DERIVED_TARGETS =

DEBIAN_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/debian

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	set -e
	rm -rf upstream-src upstream.tar.gz
	wget "$(MCTP_ARCHIVE_URL)" -O upstream.tar.gz
	# Verify the download against the pinned checksum before extracting, so a
	# corrupted or tampered tarball aborts the build (fail closed).
	if [ -z "$(MCTP_ARCHIVE_SHA256)" ]; then
		echo "ERROR: MCTP_ARCHIVE_SHA256 is unset; refusing to extract unverified $(MCTP_ARCHIVE_URL)" >&2
		exit 1
	fi
	echo "$(MCTP_ARCHIVE_SHA256)  upstream.tar.gz" | sha256sum -c -
	tar xf upstream.tar.gz
	srcdir=$$(ls -d mctp-* | head -1)
	mv "$$srcdir" upstream-src
	cp -a "$(DEBIAN_DIR)" upstream-src/debian
	# Native changelog version must match the produced .deb filename.
	{ \
		echo "mctp ($(MCTP_PKG_VERSION)) unstable; urgency=medium"; \
		echo ""; \
		echo "  * SONiC aspeed: package upstream $(MCTP_UPSTREAM_TAG)"; \
		echo ""; \
		echo " -- SONiC <sonic@microsoft.com>  $$(date -R)"; \
	} > upstream-src/debian/changelog
	pushd upstream-src
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd
	mv "$(MCTP)" "$(DEST)/"
	rm -rf upstream-src upstream.tar.gz
	rm -f ./*.changes ./*.buildinfo ./*.dsc 2>/dev/null || true

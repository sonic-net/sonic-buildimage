# Build the ispvme Debian package from the upstream NVIDIA/ispvme commit
# tarball (meson), using the debian/ metadata in this directory.

.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = $(ISPVME)
DERIVED_TARGETS =

DEBIAN_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/debian

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	set -e
	rm -rf upstream-src upstream.tar.gz ispvme-*
	wget "$(ISPVME_ARCHIVE_URL)" -O upstream.tar.gz
	# Verify the download against the pinned checksum before extracting, so a
	# corrupted or tampered tarball aborts the build (fail closed).
	if [ -z "$(ISPVME_ARCHIVE_SHA256)" ]; then
		echo "ERROR: ISPVME_ARCHIVE_SHA256 is unset; refusing to extract unverified $(ISPVME_ARCHIVE_URL)" >&2
		exit 1
	fi
	echo "$(ISPVME_ARCHIVE_SHA256)  upstream.tar.gz" | sha256sum -c -
	tar xf upstream.tar.gz
	srcdir=$$(find . -mindepth 1 -maxdepth 1 -type d -name 'ispvme-*' -print -quit)
	test -n "$$srcdir"
	mv "$$srcdir" upstream-src
	cp -a "$(DEBIAN_DIR)" upstream-src/debian
	# Native changelog version must match the produced .deb filename.
	{ \
		echo "ispvme ($(ISPVME_PKG_VERSION)) unstable; urgency=medium"; \
		echo ""; \
		echo "  * SONiC aspeed: package upstream ispvme $(ISPVME_UPSTREAM_COMMIT)"; \
		echo ""; \
		echo " -- SONiC <sonic@microsoft.com>  $$(date -R)"; \
	} > upstream-src/debian/changelog
	pushd upstream-src
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd
	mv "$(ISPVME)" "$(DEST)/"
	rm -rf upstream-src upstream.tar.gz
	# Drop incidental build leftovers (dbgsym, changes, etc.) from the recipe dir.
	rm -f ./*.deb ./*.changes ./*.buildinfo ./*.dsc 2>/dev/null || true

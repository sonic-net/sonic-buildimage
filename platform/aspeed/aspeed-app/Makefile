# Build the aspeed-app Debian package from the upstream AspeedTech-BMC
# aspeed_app repository (meson), using the debian/ metadata in this directory.

.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = $(ASPEED_APP)
DERIVED_TARGETS =

DEBIAN_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/debian

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	set -e
	rm -rf upstream-src
	git clone --recurse-submodules -b "v$(ASPEED_APP_VERSION)" --depth 1 \
		https://github.com/AspeedTech-BMC/aspeed_app.git upstream-src
	cp -a "$(DEBIAN_DIR)" upstream-src/debian
	{ \
		echo "aspeed-app ($(ASPEED_APP_PKG_VERSION)) unstable; urgency=medium"; \
		echo ""; \
		echo "  * SONiC aspeed: package upstream v$(ASPEED_APP_VERSION)"; \
		echo ""; \
		echo " -- SONiC <sonic@microsoft.com>  $$(date -R)"; \
	} > upstream-src/debian/changelog
	pushd upstream-src
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd
	mv "$(ASPEED_APP)" "$(DEST)/"
	rm -rf upstream-src
	rm -f ./*.changes ./*.buildinfo ./*.dsc 2>/dev/null || true

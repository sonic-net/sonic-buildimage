SHELL := /bin/bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c

VERSION := 1.2.0
DEBIAN_VERSION := $(VERSION)-6
PACKAGE_VERSION := $(DEBIAN_VERSION)+sonic1
SOURCE_POOL := https://deb.debian.org/debian/pool/main/h/hiredis
MAIN_TARGET := libhiredis1.1.0_$(PACKAGE_VERSION)_$(CONFIGURED_ARCH).deb
DEV_TARGET := libhiredis-dev_$(PACKAGE_VERSION)_$(CONFIGURED_ARCH).deb
DBG_TARGET := libhiredis1.1.0-dbgsym_$(PACKAGE_VERSION)_$(CONFIGURED_ARCH).deb
BUILD_DIR := $(CURDIR)/.build

$(DEST)/$(MAIN_TARGET): Makefile source.sha256 changelog.sonic patches/atomic-close-on-exec.patch
	mkdir -p $(BUILD_DIR) $(DEST)
	cd $(BUILD_DIR)
	for archive in hiredis_$(VERSION).orig.tar.gz hiredis_$(DEBIAN_VERSION).debian.tar.xz hiredis_$(DEBIAN_VERSION).dsc; do
	    if [ ! -f "$$archive" ]; then
	        curl --fail --location --retry 3 -o "$$archive" "$(SOURCE_POOL)/$$archive"
	    fi
	done
	sha256sum --check $(CURDIR)/source.sha256
	rm -rf hiredis-$(VERSION)
	dpkg-source -x hiredis_$(DEBIAN_VERSION).dsc
	cd hiredis-$(VERSION)
	cp $(CURDIR)/patches/atomic-close-on-exec.patch debian/patches/
	printf '%s\n' atomic-close-on-exec.patch >> debian/patches/series
	cat $(CURDIR)/changelog.sonic debian/changelog > debian/changelog.sonic
	mv debian/changelog.sonic debian/changelog
ifeq ($(CROSS_BUILD_ENVIRON),y)
	dpkg-buildpackage -us -uc -b -d -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(if $(SONIC_CONFIG_MAKE_JOBS),$(SONIC_CONFIG_MAKE_JOBS),2)
else
	dpkg-buildpackage -us -uc -b -j$(if $(SONIC_CONFIG_MAKE_JOBS),$(SONIC_CONFIG_MAKE_JOBS),2) $(if $(SONIC_DPKG_ADMINDIR),--admindir $(SONIC_DPKG_ADMINDIR),)
endif
	cd ..
	cp $(MAIN_TARGET) $(DEV_TARGET) $(DBG_TARGET) $(DEST)/

$(DEST)/$(DEV_TARGET) $(DEST)/$(DBG_TARGET): $(DEST)/$(MAIN_TARGET)

.PHONY: check
check:
	./tests/close-on-exec

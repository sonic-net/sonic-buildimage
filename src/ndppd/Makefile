.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(NDPPD)
DERIVED_TARGETS = $(NDPPD_DBG)

NDPPD_URL = https://deb.debian.org/debian/pool/main/n/ndppd
DSC_FILE = ndppd_$(NDPPD_VERSION_FULL).dsc

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale source tree.
	rm -rf ndppd-$(NDPPD_VERSION)

	# Download and unpack the pinned Debian source package.
	dget $(NDPPD_URL)/$(DSC_FILE)

	pushd ndppd-$(NDPPD_VERSION)
	patch -p1 < ../patch/0001-validate-neighbor-message-lengths.patch

	# Build the patched Debian packages.
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -Pnocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	# Move the newly built packages to the destination directory.
	# Debian emits automatic debug packages as .ddeb; SONiC tracks them as .deb.
	mv $(NDPPD_DBG:.deb=.ddeb) $(NDPPD_DBG)
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

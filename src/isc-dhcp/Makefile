.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(ISC_DHCP_RELAY)
DERIVED_TARGETS = $(ISC_DHCP_RELAY_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./isc-dhcp-$(ISC_DHCP_VERSION)

	# Get isc-dhcp release, debian files
	dget -u http://deb.debian.org/debian/pool/main/i/isc-dhcp/isc-dhcp_$(ISC_DHCP_VERSION_FULL).dsc
	pushd ./isc-dhcp-$(ISC_DHCP_VERSION)

	# Create a git repository here for stg to apply patches
	git init
	git add -f *
	git commit -qm "initial commit"

	# Apply patches
	stg init
	stg import -s ../patch/series

	# resolute: bind 9.11.36 (vendored in isc-dhcp) link error with -flto=auto:
	# libisc.a has the symbols (T) but svtest/dhclient link reports undefined
	# reference. Disable LTO for this package (CFLAGS + LDFLAGS).
	export DEB_CFLAGS_MAINT_STRIP="-flto=auto -ffat-lto-objects"
	export DEB_LDFLAGS_MAINT_STRIP="-flto=auto -ffat-lto-objects"

	# Build source and Debian packages
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	# Move the newly-built .deb packages to the destination directory
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

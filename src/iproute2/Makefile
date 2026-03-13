.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = iproute2_$(IPROUTE2_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = iproute2-dbgsym_$(IPROUTE2_VERSION_SONIC)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtaining the iproute2 source
	rm -rf ./iproute2-$(IPROUTE2_VERSION_BASE)
	dget http://debian-archive.trafficmanager.net/debian/pool/main/i/iproute2/iproute2_$(IPROUTE2_VERSION).dsc
	pushd iproute2-$(IPROUTE2_VERSION_BASE)

	# Create a git repository here for stg to apply patches
	git init
	git add -f *
	git commit -qm "initial commit"

	# Apply patch series
	stg init
	stg import -s ../patch/series

	# Remove trixie-only build deps not available in bookworm
	sed -i '/^include \/usr\/share\/debhelper\/dh_package_notes\/package-notes.mk/d' debian/rules
	sed -i '/dh-sequence-movetousr,/d' debian/control
	sed -i '/dh-package-notes,/d' debian/control

	# Update version to match SONiC naming convention
	sed -i '1s/($(IPROUTE2_VERSION))/($(IPROUTE2_VERSION_SONIC))/' debian/changelog

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -d -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -d -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

	# Clean up build artifacts
	rm -rf ./iproute2-$(IPROUTE2_VERSION_BASE)
	rm -f ./iproute2_$(IPROUTE2_VERSION).dsc
	rm -f ./iproute2_$(IPROUTE2_VERSION_BASE).orig.tar.xz
	rm -f ./iproute2_$(IPROUTE2_VERSION).debian.tar.xz
	rm -f ./iproute2_*.buildinfo ./iproute2_*.changes

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

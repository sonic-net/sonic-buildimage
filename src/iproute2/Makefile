.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(IPROUTE2)
DERIVED_TARGETS = $(IPROUTE2_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Build from Salsa (Debian) submodule which includes debian/ packaging
	pushd ./iproute2

	# Ensure clean state
	git checkout -- . 2>/dev/null || true
	git clean -xfdf

	# Append SONiC patches to Debian's quilt series
	for p in ../patch/*.patch; do \
		[ -f "$$p" ] || continue; \
		cp "$$p" debian/patches/; \
		echo "$$(basename $$p)" >> debian/patches/series; \
	done

	# Update changelog version to SONiC convention
	sed -i "1s/($(IPROUTE2_VERSION)-[^)]*)/\($(IPROUTE2_VERSION)-$(IPROUTE2_SUBVERSION)\)/" debian/changelog

	# Remove dh sequences unavailable in sonic-slave build environment
	sed -i '/dh-package-notes/d; /dh-sequence-movetousr/d' debian/control
	sed -i '\|/usr/share/debhelper/dh_package_notes/package-notes.mk|d' debian/rules

	# Build
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -d -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -d -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	# Restore submodule to clean state
	git checkout -- . 2>/dev/null || true
	git clean -xfdf
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

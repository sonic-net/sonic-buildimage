.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(IPROUTE2)
DERIVED_TARGETS = $(IPROUTE2_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Build from Debian source packaging, using the distro-specific iproute2 version.
	rm -rf ./iproute2-$(IPROUTE2_VERSION_BASE)
	dget -u https://deb.debian.org/debian/pool/main/i/iproute2/iproute2_$(IPROUTE2_DEBIAN_VERSION).dsc
	pushd ./iproute2-$(IPROUTE2_VERSION_BASE)

	# Create a git repository here for stg to apply patches.
	git init
	git add -f *
	git commit -qm "initial commit"

	# Apply SONiC patches in explicit series order.
	if grep -q '[^[:space:]]' ../patch/series; then
		stg init
		stg import -s ../patch/series
	fi

	# Bump the Debian package version so the locally-built package wins over distro iproute2.
	sed -i "1s/($(IPROUTE2_DEBIAN_VERSION))/($(IPROUTE2_VERSION_FULL))/" debian/changelog

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $* $(DEST)/
	for deb in $(DERIVED_TARGETS); do \
		[ ! -f "$$deb" ] || mv "$$deb" $(DEST)/; \
	done

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = libteam5_$(LIBTEAM_VERSION_FULL)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = libteam-dev_$(LIBTEAM_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libteamdctl0_$(LIBTEAM_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libteam-utils_$(LIBTEAM_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libteam5-dbgsym_$(LIBTEAM_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libteamdctl0-dbgsym_$(LIBTEAM_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  libteam-utils-dbgsym_$(LIBTEAM_VERSION_FULL)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./libteam-$(LIBTEAM_VERSION)

	# Get libteam release, debian files
	dget -u https://deb.debian.org/debian/pool/main/libt/libteam/libteam_$(LIBTEAM_VERSION_FULL).dsc
	pushd ./libteam-$(LIBTEAM_VERSION)

	# Create a git repository here for stg to apply patches
	git init
	git add -f *
	git commit -qm "initial commit"

	# Apply patches
	stg init
	stg import -s ../patch/series

	# Build source and Debian packages
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = libnl-3-200_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = libnl-3-200-dbgsym_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-3-dev_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-genl-3-200_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-genl-3-200-dbgsym_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-genl-3-dev_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-route-3-200_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-route-3-200-dbgsym_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-route-3-dev_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb  \
		  libnl-nf-3-200_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-nf-3-200-dbgsym_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-nf-3-dev_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-cli-3-200_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-cli-3-200-dbgsym_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb \
		  libnl-cli-3-dev_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtaining the libnl3
	rm -rf ./libnl3-$(LIBNL3_VERSION_BASE)
	dget https://deb.debian.org/debian/pool/main/libn/libnl3/libnl3_$(LIBNL3_VERSION).dsc
	pushd libnl3-$(LIBNL3_VERSION_BASE)

	# Create a git repository here for stg to apply patches
	git init
	git add -f *
	git commit -qm "initial commit"

	# Apply patch series
	stg init
	stg import -s ../patch/series

ifeq ($(CROSS_BUILD_ENVIRON), y)
	DPKG_GENSYMBOLS_CHECK_LEVEL=0 dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	DPKG_GENSYMBOLS_CHECK_LEVEL=0 dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

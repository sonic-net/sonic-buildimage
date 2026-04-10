.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = libtcmalloc-minimal4_$(GPERFTOOLS_VERSION)-$(GPERFTOOLS_SUBVERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = libgoogle-perftools-dev_$(GPERFTOOLS_VERSION)-$(GPERFTOOLS_SUBVERSION)_$(CONFIGURED_ARCH).deb \
		  libgoogle-perftools4_$(GPERFTOOLS_VERSION)-$(GPERFTOOLS_SUBVERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtaining the gperftools (google-perftools)
	rm -rf ./google-perftools-$(GPERFTOOLS_VERSION)
	dget https://deb.debian.org/debian/pool/main/g/google-perftools/google-perftools_$(GPERFTOOLS_VERSION)-$(GPERFTOOLS_SUBVERSION).dsc
	pushd google-perftools-$(GPERFTOOLS_VERSION)

	# Create a git repository here for stg to apply patches
	git init
	git add -f *
	git commit -qm "initial commit"

	# Apply patch series (if any)
	stg init
	stg import -s ../patch/series

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

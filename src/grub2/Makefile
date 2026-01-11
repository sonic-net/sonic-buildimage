.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = grub2-common_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = grub-efi_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS += grub-common_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb

ifeq ($(CONFIGURED_ARCH),amd64)
DERIVED_TARGETS += grub-pc-bin_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS += grub-efi-amd64_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS += grub-efi-amd64-bin_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
else ifeq ($(CONFIGURED_ARCH),arm64)
DERIVED_TARGETS += grub-efi-arm64_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS += grub-efi-arm64-bin_$(GRUB2_VERSION)_$(CONFIGURED_ARCH).deb
endif

SUFFIX := $(shell date +%Y%m%d\.%H%M%S)
STG_BRANCH := stg_temp.$(SUFFIX)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Get grub2 debian packaging
	export
	rm -rf ./grub2
	git clone https://salsa.debian.org/grub-team/grub.git ./grub2

	pushd ./grub2
	git checkout debian/$(GRUB2_VERSION)
	git checkout -b $(STG_BRANCH)
	stg init
	stg import -s ../patch/series

	# Build packages
	# Unset the MAKEFLAGS and MFLAGS variables to remove the jobserver information
	# from the environment variable. Without this, the GRUB targets that Debian
	# tries to build will get built in parallel, which will cause issues.
	# (Parallelism is limited to compilation within a target, but not multiple
	# targets being built at the same time.)
	unset MAKEFLAGS MFLAGS
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

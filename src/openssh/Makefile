.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = openssh-server_$(OPENSSH_VERSION_FULL)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = openssh-server-dbgsym_$(OPENSSH_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
				  openssh-client_$(OPENSSH_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
				  openssh-client-dbgsym_$(OPENSSH_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
				  openssh-sftp-server_$(OPENSSH_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
				  openssh-sftp-server-dbgsym_$(OPENSSH_VERSION_FULL)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./openssh-$(OPENSSH_VERSION)

	# Get openssh release, debian files
	dget https://deb.debian.org/debian/pool/main/o/openssh/openssh_$(OPENSSH_VERSION_FULL).dsc
	pushd ./openssh-$(OPENSSH_VERSION)

	# Create a git repository here for stg to apply patches
	git init
	git add -f *
	git commit -qm "initial commit"

	# Apply patch series
	stg init
	stg import -s ../patch/series

	# Build package
ifeq ($(CROSS_BUILD_ENVIRON), y)
	patch -p1 < ../patch/cross-compile-changes.patch
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

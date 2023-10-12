.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = openssh-server_$(OPENSSH_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = openssh-server-dbgsym_$(OPENSSH_VERSION)_$(CONFIGURED_ARCH).deb \
				  openssh-client_$(OPENSSH_VERSION)_$(CONFIGURED_ARCH).deb \
				  openssh-client-dbgsym_$(OPENSSH_VERSION)_$(CONFIGURED_ARCH).deb \
				  openssh-sftp-server_$(OPENSSH_VERSION)_$(CONFIGURED_ARCH).deb \
				  openssh-sftp-server-dbgsym_$(OPENSSH_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtain openssh: https://salsa.debian.org/ssh-team/openssh/-/tree/debian/1%258.4p1-5
	rm -rf ./openssh-server
	git clone https://salsa.debian.org/ssh-team/openssh.git openssh-server
	pushd ./openssh-server

	# Check out tag: debian/1%8.4p1-5
	git checkout -b openssh-src -f debian/1\%$(OPENSSH_VERSION)

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

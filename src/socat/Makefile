.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = socat_$(SOCAT_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = socat-dbgsym_$(SOCAT_VERSION)_$(CONFIGURED_ARCH).deb
SOCAT_UPSTREAM_VERSION = $(word 1,$(subst -, ,$(SOCAT_VERSION)))

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./socat-$(SOCAT_UPSTREAM_VERSION)

	# Get source package
	wget -NO socat_$(SOCAT_VERSION).dsc "$(BUILD_PUBLIC_URL)/debian/socat_$(SOCAT_VERSION).dsc"
	wget -NO socat_$(SOCAT_VERSION).debian.tar.xz "$(BUILD_PUBLIC_URL)/debian/socat_$(SOCAT_VERSION).debian.tar.xz"
	wget -NO socat_$(SOCAT_UPSTREAM_VERSION).orig.tar.bz2 "$(BUILD_PUBLIC_URL)/debian/socat_$(SOCAT_UPSTREAM_VERSION).orig.tar.bz2"

	dpkg-source -x socat_$(SOCAT_VERSION).dsc

	# Build source and Debian packages
	pushd socat-$(SOCAT_UPSTREAM_VERSION)
	patch -p0 < ../patch/enable_readline.patch
	# Share the source fix already used by the registered Bazel producer.
	# Keep the patch in its source owner and verify the immutable download.
	wget -O procan-namespace-nul.patch "https://raw.githubusercontent.com/sonic-net/sonic-bazel-registry/f9a9baaeabad4a77c3ad5939fe257434e635e0dc/modules/socat/1.8.0.2.sonic.8/overlay/bazel/0003-procan-namespace-nul.patch"
	echo "3772cdaa0a90488d3eca0a51d07a8ddc4e7d0dc202159b602c787555b7fb4dc8  procan-namespace-nul.patch" | sha256sum -c -
	patch -p1 < procan-namespace-nul.patch
	rm procan-namespace-nul.patch
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -rfakeroot -Pnocheck -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	# Move the newly-built .deb packages to the destination directory
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)


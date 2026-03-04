.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = rasdaemon_$(RASDAEMON_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = rasdaemon-dbgsym_$(RASDAEMON_VERSION)_$(CONFIGURED_ARCH).deb

# Pinned commit hash for debian/0.6.8-1 tag.
# We use a commit hash instead of a tarball because the patch is applied
# via git apply, which requires a git repository.
RASDAEMON_COMMIT = 51a7f485f8b2e2ae43e613f19c5a387595174132

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf rasdaemon/

	# Checkout Repository (pinned to specific commit for reproducibility)
	git clone https://salsa.debian.org/tai271828/rasdaemon.git
	pushd ./rasdaemon
	git checkout $(RASDAEMON_COMMIT)

	# Patch
	git apply ../0001-Check-CPUs-online-not-configured.patch
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

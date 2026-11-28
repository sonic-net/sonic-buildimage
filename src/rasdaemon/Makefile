.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = rasdaemon_$(RASDAEMON_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = rasdaemon-dbgsym_$(RASDAEMON_VERSION)_$(CONFIGURED_ARCH).deb

# Pinned commit hash for debian/0.6.8-1 tag.
RASDAEMON_COMMIT = 51a7f485f8b2e2ae43e613f19c5a387595174132

# Preserve the published Debian 0.6.8-1 source, packaging and patch series.
# This revision is no longer in the current pool; use its immutable snapshot.
RASDAEMON_SOURCE_URL = https://snapshot.debian.org/archive/debian/20230206T092119Z/pool/main/r/rasdaemon
DSC_FILE = rasdaemon_$(RASDAEMON_VERSION).dsc
ORIG_FILE = rasdaemon_$(firstword $(subst -, ,$(RASDAEMON_VERSION))).orig.tar.gz
DEBIAN_FILE = rasdaemon_$(RASDAEMON_VERSION).debian.tar.xz
# Update these official source checksums together with RASDAEMON_VERSION.
RASDAEMON_DSC_SHA256 = 50009d1318f897f01f24264511b2c7f90a5626447b40e42b488fedd54c750da9
RASDAEMON_ORIG_SHA256 = d7c9ad61a13ca230e376db4014ae3192992eff10908adf38fad1917609f7d341
RASDAEMON_DEBIAN_SHA256 = 651e13bf4bfb489dce0f427a46bf2ee015548c30fcfec32c544d98806225e63f

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf rasdaemon/

ifeq ($(PREFER_DEBIAN_REPO), y)
	wget -O $(DSC_FILE) $(RASDAEMON_SOURCE_URL)/$(DSC_FILE)
	wget -O $(ORIG_FILE) $(RASDAEMON_SOURCE_URL)/$(ORIG_FILE)
	wget -O $(DEBIAN_FILE) $(RASDAEMON_SOURCE_URL)/$(DEBIAN_FILE)
	echo "$(RASDAEMON_DSC_SHA256)  $(DSC_FILE)" | sha256sum -c -
	echo "$(RASDAEMON_ORIG_SHA256)  $(ORIG_FILE)" | sha256sum -c -
	echo "$(RASDAEMON_DEBIAN_SHA256)  $(DEBIAN_FILE)" | sha256sum -c -
	# Match the Git recipe: apply SONiC's patch before dpkg-buildpackage applies
	# Debian's quilt series during its --before-build preparation.
	dpkg-source --skip-patches -x $(DSC_FILE) rasdaemon
else
	# Checkout Repository (pinned to specific commit for reproducibility)
	git clone https://salsa.debian.org/tai271828/rasdaemon.git
endif
	pushd ./rasdaemon

	# Patch
ifeq ($(PREFER_DEBIAN_REPO), y)
	patch --batch --forward -p1 < ../0001-Check-CPUs-online-not-configured.patch
else
	git checkout $(RASDAEMON_COMMIT)
	git apply ../0001-Check-CPUs-online-not-configured.patch
endif
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

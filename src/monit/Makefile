.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = monit_$(MONIT_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = monit-dbgsym_$(MONIT_VERSION)_$(CONFIGURED_ARCH).deb

MONIT_URL = https://deb.debian.org/debian/pool/main/m/monit
DSC_FILE = monit_$(MONIT_VERSION).dsc
ORIG_FILE = monit_$(firstword $(subst -, ,$(MONIT_VERSION))).orig.tar.gz
DEBIAN_FILE = monit_$(MONIT_VERSION).debian.tar.xz

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./monit

ifeq ($(PREFER_DEBIAN_REPO), y)
	# Use Debian's published source release, including its packaging and patches.
	wget -O $(DSC_FILE) $(MONIT_URL)/$(DSC_FILE)
	wget -O $(ORIG_FILE) $(MONIT_URL)/$(ORIG_FILE)
	wget -O $(DEBIAN_FILE) $(MONIT_URL)/$(DEBIAN_FILE)
	echo "$(MONIT_DSC_SHA256)  $(DSC_FILE)" | sha256sum -c -
	echo "$(MONIT_ORIG_SHA256)  $(ORIG_FILE)" | sha256sum -c -
	echo "$(MONIT_DEBIAN_SHA256)  $(DEBIAN_FILE)" | sha256sum -c -
	# Keep the same patch order as the Git recipe: SONiC's StGit series first,
	# then Debian's quilt series during dpkg-buildpackage --before-build.
	dpkg-source --skip-patches -x $(DSC_FILE) monit
else
	# Clone monit repo
	git clone https://salsa.debian.org/debian/monit.git
endif
	pushd ./monit

ifeq ($(PREFER_DEBIAN_REPO), y)
	git init
	git add -f *
	git commit -m "unmodified Debian monit source"
else
	# Reset HEAD to the commit of the proper tag
	# NOTE: Using "git checkout <tag_name>" here detaches our HEAD,
	# which stg doesn't like, so we use this method instead
	# NOTE: For some reason, tags in the Debian monit repo are prefixed with "1%"
	git reset --hard debian/1\%$(MONIT_VERSION)
endif

	# Apply patches
	stg init
	stg import -s ../patch/series

ifeq ($(CROSS_BUILD_ENVIRON), y)
	patch -p1 < ../patch/cross-compile-changes.patch
endif

	# Build source and Debian packages
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc --host-arch $(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	# Move the newly-built .deb packages to the destination directory
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -xe

SCOUTY_URL = https://github.com/r12f/scouty/archive/refs/tags/v$(SCOUTY_VERSION).tar.gz

SCOUTY_TARGET = sonic-scouty_$(SCOUTY_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(SCOUTY_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf scouty-$(SCOUTY_VERSION)
	rm -f scouty-$(SCOUTY_VERSION).tar.gz

	# Download and verify source
	curl -sL $(SCOUTY_URL) -o scouty-$(SCOUTY_VERSION).tar.gz
	echo "$(SCOUTY_SHA256)  scouty-$(SCOUTY_VERSION).tar.gz" | sha256sum -c -
	tar xzf scouty-$(SCOUTY_VERSION).tar.gz

	# Copy debian packaging and SONiC files into source tree
	cp -r debian scouty-$(SCOUTY_VERSION)/
	cp scouty.sh scouty-$(SCOUTY_VERSION)/
	cp config.yaml scouty-$(SCOUTY_VERSION)/

	# Update changelog version to match SCOUTY_VERSION (in the copied tree, not the repo)
	sed -i "1s/^sonic-scouty ([^)]*)/sonic-scouty ($(SCOUTY_VERSION))/" scouty-$(SCOUTY_VERSION)/debian/changelog

	pushd scouty-$(SCOUTY_VERSION)

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -us -uc -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	test -f $* || { echo "ERROR: $* not found after build"; exit 1; }
	mv $* $(DEST)/

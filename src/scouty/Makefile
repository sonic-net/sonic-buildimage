SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -xe

SCOUTY_URL = https://github.com/r12f/scouty/archive/refs/tags/v$(SCOUTY_VERSION).tar.gz

SCOUTY_TARGET = sonic-scouty_$(SCOUTY_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(SCOUTY_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf scouty-$(SCOUTY_VERSION)

	# Download and extract source
	curl -sL $(SCOUTY_URL) -o scouty-$(SCOUTY_VERSION).tar.gz
	tar xzf scouty-$(SCOUTY_VERSION).tar.gz

	# Copy debian packaging and SONiC files into source tree
	cp -r debian scouty-$(SCOUTY_VERSION)/
	cp scouty.sh scouty-$(SCOUTY_VERSION)/
	cp config.yaml scouty-$(SCOUTY_VERSION)/

	pushd scouty-$(SCOUTY_VERSION)

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -us -uc -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $* $(DEST)/

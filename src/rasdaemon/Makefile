.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = rasdaemon_$(RASDAEMON_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = rasdaemon-dbgsym_$(RASDAEMON_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf rasdaemon/

	# Checkout Repository
	git clone https://salsa.debian.org/tai271828/rasdaemon.git -b debian/$(RASDAEMON_VERSION)

	pushd ./rasdaemon
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

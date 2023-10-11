.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(HSFLOWD)
DERIVED_TARGET = $(HSFLOWD_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -fr ./host-sflow
	git clone https://github.com/sflow/host-sflow

	pushd ./host-sflow
	git checkout -b sflow tags/v$(HSFLOWD_VERSION)-$(HSFLOWD_SUBVERSION)

        # Apply patch series
	stg init
	stg import -s ../patch/series

	mkdir -p debian
	cp -r DEBIAN_build/* debian
	chmod u+x debian/rules
	sed -i -e s/_VERSION_/$(HSFLOWD_VERSION)-$(HSFLOWD_SUBVERSION)/g debian/changelog

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR) --buildinfo-option=-u. --changes-option=-u.
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR) --buildinfo-option=-u. --changes-option=-u.
endif

	mv $(DERIVED_TARGET) $* $(DEST)/
	popd

$(addprefix $(DEST)/, $(DERIVED_TARGET)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

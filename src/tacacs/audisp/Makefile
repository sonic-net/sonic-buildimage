.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = audisp-tacplus_$(AUDISP_TACPLUS_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = audisp-tacplus-dbgsym_$(AUDISP_TACPLUS_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtain audisp-tacplus
	rm -rf ./audisp-tacplus

	git clone https://github.com/daveolson53/audisp-tacplus.git

	# checkout by sha1
	pushd ./audisp-tacplus
	git checkout 559c9f22edd4f2dea0ecedffb3ad9502b12a75b6

	# Apply patches
	cp -r ../patches patches
	quilt push -a

	# fix aclocal depency issue by run auto.sh
	./auto.sh

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	# build package
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = libnss-tacplus_$(NSS_TACPLUS_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = libnss-tacplus-dbgsym_$(NSS_TACPLUS_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtain libnss-tacplus
	rm -rf ./libnss-tacplus
	git clone https://github.com/daveolson53/libnss-tacplus.git
	pushd ./libnss-tacplus
	git checkout -b test -f 19008ab

	# Apply patch
	stg init
	stg import -s ../patch/series

	# Import optional local patch series ../patch/series.local, if present.
	# No-op upstream (no series.local is shipped); lets downstreams add local
	# patches without editing the shared series.
	if [ -f ../patch/series.local ]; then
		stg import -s ../patch/series.local
	fi

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

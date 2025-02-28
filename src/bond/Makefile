.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(BOND)
DERIVED_TARGETS = 

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./bond-$(BOND_VERSION)

	# Clone bond
	git clone -b $(BOND_VERSION) https://github.com/Microsoft/bond.git bond-$(BOND_VERSION)

	pushd bond-$(BOND_VERSION)
	git checkout -b $(BOND_VERSION)-patches

	# Apply patches
	stg init
ifeq ($(shell dpkg --print-architecture),armhf)
	stg import -s ../patch/series_armhf
else
	stg import -s ../patch/series
endif

	# Build source and Debian packages
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd

	# Move the newly-built .deb packages to the destination directory
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

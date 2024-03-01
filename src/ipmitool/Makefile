.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(IPMITOOL)
DERIVED_TARGETS = $(IPMITOOL_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./ipmitool-$(IPMITOOL_VERSION) ./ipmitool*.{deb,udeb,dsc,gz,xz}
	# Obtain ipmitool
	dget https://deb.debian.org/debian/pool/main/i/ipmitool/ipmitool_$(IPMITOOL_VERSION_FULL).dsc
	# Build source and Debian packages
	pushd ipmitool-$(IPMITOOL_VERSION)

	git init
	git add -f *
	git commit -m "unmodified ipmitool source"

	# Apply patches
	stg init
	stg import -s ../patch/series
	dpkg-buildpackage -rfakeroot -d -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd
	mv $* $(DERIVED_TARGETS) $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)


.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(IFUPDOWN2)


$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./ifupdown2-$(IFUPDOWN2_VERSION) ./$(IFUPDOWN2_VERSION).tar.gz

	# Get ifupdown2 release
	wget -O $(IFUPDOWN2_VERSION).tar.gz https://github.com/CumulusNetworks/ifupdown2/archive/$(IFUPDOWN2_VERSION).tar.gz
	echo "$(IFUPDOWN2_SHA256)  $(IFUPDOWN2_VERSION).tar.gz" | sha256sum -c -
	tar -z -f $(IFUPDOWN2_VERSION).tar.gz -x
	pushd ./ifupdown2-$(IFUPDOWN2_VERSION)

	git init
	git add -f *
	git commit -m "unmodified ifupdown2 source"

	# Apply patch series
	stg init
	stg import -s ../patch/series

	# Build source and Debian packages
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -d -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	# Move the newly-built .deb packages to the destination directory
	mv $* $(DEST)/

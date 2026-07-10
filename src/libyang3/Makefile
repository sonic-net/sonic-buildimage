.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

LIBYANG_URL = http://debian-archive.trafficmanager.net/debian/pool/main/liby/libyang

MAIN_TARGET = $(LIBYANG3)
DERIVED_TARGETS = $(LIBYANG3_DEV) $(LIBYANG3_DBG) $(LIBYANG3_TOOLS) $(LIBYANG3_TOOLS_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtaining the libyang
	rm -fr ./libyang-$(LIBYANG3_VERSION)

	# download debian libyang
	dget -u $(LIBYANG_URL)/libyang_$(LIBYANG3_FULLVERSION).dsc

	pushd libyang-$(LIBYANG3_VERSION)
	# The package libyang3.12.2 is taken from debian trixie

	# Enable large file support for 32-bit arch
	echo 'add_definitions(-D_FILE_OFFSET_BITS=64)' >> CMakeLists.txt

	# Apply patch series
	patch -p1 < ../patch/0001-pr2362-lyd_validate_noextdeps.patch

	# Debian buster and below do not have new enough versions of xxhash and
	# build fails.  xxhash is an optimization, not a real dependency.
	if [ "`lsb_release -r | awk '{ print $$2 }'`" -le "10" ] ; then \
		sed -i -e '/.*libxxhash.*/d' debian/control; \
		sed -i -e 's/^find_package(XXHash)/#find_package(XXHash)/' CMakeLists.txt; \
	fi

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -d -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	popd

	# Move the newly-built .deb packages to the destination directory
	mv $* $(DERIVED_TARGETS) $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

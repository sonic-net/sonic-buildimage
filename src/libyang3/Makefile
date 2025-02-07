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
	# The package libyang3.7.8 is taken from debian trixie

	# Enable large file support for 32-bit arch
	echo 'add_definitions(-D_FILE_OFFSET_BITS=64)' >> CMakeLists.txt
	[ -d ".pc" ] && rm -rf .pc
	QUILT_PATCHES=../patch quilt push -a

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -d -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	popd

	# Move the newly-built .deb packages to the destination directory
	mv $* $(DERIVED_TARGETS) $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

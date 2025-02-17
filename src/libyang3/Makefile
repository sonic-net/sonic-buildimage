.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

LIBYANG_URL = http://debian-archive.trafficmanager.net/debian/pool/main/liby/libyang

DSC_FILE = libyang_$(LIBYANG3_FULLVERSION).dsc
ORIG_FILE = libyang_$(LIBYANG3_VERSION).orig.tar.xz
DEBIAN_FILE = libyang_$(LIBYANG3_FULLVERSION).debian.tar.xz

DSC_FILE_URL = $(LIBYANG_URL)/$(DSC_FILE)
ORIG_FILE_URL = $(LIBYANG_URL)/$(ORIG_FILE)
DEBIAN_FILE_URL = $(LIBYANG_URL)/$(DEBIAN_FILE)

MAIN_TARGET = $(LIBYANG3)
DERIVED_TARGETS = $(LIBYANG3_DEV) $(LIBYANG3_DBG) $(LIBYANG3_TOOLS) $(LIBYANG3_TOOLS_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
        # Obtaining the libyang
	rm -fr ./libyang-$(LIBYANG3_VERSION)

	# download debian libyang
	wget -NO "$(DSC_FILE)" $(DSC_FILE_URL)
	wget -NO "$(ORIG_FILE)" $(ORIG_FILE_URL)
	wget -NO "$(DEBIAN_FILE)" $(DEBIAN_FILE_URL)
	dpkg-source -x libyang_$(LIBYANG3_FULLVERSION).dsc

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

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(LIBYANG2)
DERIVED_TARGETS = $(LIBYANG2_DEV) $(LIBYANG2_DBG) $(LIBYANG2_TOOLS) $(LIBYANG2_TOOLS_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
        # Obtaining the libyang
	rm -fr ./libyang2-$(LIBYANG2_VERSION)

	# download debian libyang
	dget https://sonicstorage.blob.core.windows.net/debian/pool/main/liby/libyang/libyang2_$(LIBYANG2_FULLVERSION).dsc

	pushd libyang2-$(LIBYANG2_VERSION)
	#sed -i 's/set(LIBYANG_MAJOR_SOVERSION 1)/set(LIBYANG_MAJOR_SOVERSION 2)/' CMakeLists.txt
	#sed -i 's/libyang2/libyang2/' debian/libyang2.install
	# Enable large file support for 32-bit arch
	echo 'add_definitions(-D_FILE_OFFSET_BITS=64)' >> CMakeLists.txt

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -d -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	popd

	# Move the newly-built .deb packages to the destination directory
	mv $* $(DERIVED_TARGETS) $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

# libyang2 source for buster pulled from Debian mirror (archive URL 404s)
# Try multiple sources: packages.trafficmanager.net, deb.debian.org, archive.debian.org
LIBYANG_URL = https://packages.trafficmanager.net/public/debian/pool/main/liby/libyang
LIBYANG_URL_DEB = http://deb.debian.org/debian/pool/main/liby/libyang
LIBYANG_URL_ARCHIVE = http://archive.debian.org/debian/pool/main/liby/libyang

DSC_FILE = libyang2_$(LIBYANG2_FULLVERSION).dsc
ORIG_FILE = libyang2_$(LIBYANG2_VERSION).orig.tar.gz
DEBIAN_FILE = libyang2_$(LIBYANG2_FULLVERSION).debian.tar.xz

DSC_FILE_URL = $(LIBYANG_URL)/$(DSC_FILE)
ORIG_FILE_URL = $(LIBYANG_URL)/$(ORIG_FILE)
DEBIAN_FILE_URL = $(LIBYANG_URL)/$(DEBIAN_FILE)

MAIN_TARGET = $(LIBYANG2)
DERIVED_TARGETS = $(LIBYANG2_DEV) $(LIBYANG2_DBG) $(LIBYANG2_TOOLS) $(LIBYANG2_TOOLS_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
        # Obtaining the libyang
	rm -fr ./libyang2-$(LIBYANG2_VERSION)

	# download debian libyang with fallback sources
	# Try packages.trafficmanager.net first, then deb.debian.org, then archive.debian.org
	if ! wget -NO "$(DSC_FILE)" $(DSC_FILE_URL) 2>/dev/null; then \
		if ! wget -NO "$(DSC_FILE)" $(LIBYANG_URL_DEB)/$(DSC_FILE) 2>/dev/null; then \
			wget -NO "$(DSC_FILE)" $(LIBYANG_URL_ARCHIVE)/$(DSC_FILE); \
		fi; \
	fi
	\
	if ! wget -NO "$(ORIG_FILE)" $(ORIG_FILE_URL) 2>/dev/null; then \
		if ! wget -NO "$(ORIG_FILE)" $(LIBYANG_URL_DEB)/$(ORIG_FILE) 2>/dev/null; then \
			wget -NO "$(ORIG_FILE)" $(LIBYANG_URL_ARCHIVE)/$(ORIG_FILE); \
		fi; \
	fi
	\
	if ! wget -NO "$(DEBIAN_FILE)" $(DEBIAN_FILE_URL) 2>/dev/null; then \
		if ! wget -NO "$(DEBIAN_FILE)" $(LIBYANG_URL_DEB)/$(DEBIAN_FILE) 2>/dev/null; then \
			wget -NO "$(DEBIAN_FILE)" $(LIBYANG_URL_ARCHIVE)/$(DEBIAN_FILE); \
		fi; \
	fi
	\
	dpkg-source -x libyang2_$(LIBYANG2_FULLVERSION).dsc

	pushd libyang2-$(LIBYANG2_VERSION)
	#sed -i 's/set(LIBYANG_MAJOR_SOVERSION 1)/set(LIBYANG_MAJOR_SOVERSION 2)/' CMakeLists.txt
	#sed -i 's/libyang2/libyang2/' debian/libyang2.install
	# Enable large file support for 32-bit arch
	echo 'add_definitions(-D_FILE_OFFSET_BITS=64)' >> CMakeLists.txt

	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd

	# Move the newly-built .deb packages to the destination directory
	mv $* $(DERIVED_TARGETS) $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

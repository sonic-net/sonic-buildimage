.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(LLDPD)
DERIVED_TARGETS = $(LIBLLDPCTL) $(LLDPD_DBG)

LLDP_URL = $(BUILD_PUBLIC_URL)/debian/pool/main/l/lldpd

DSC_FILE = lldpd_$(LLDPD_VERSION_FULL).dsc
ORIG_FILE = lldpd_$(LLDPD_VERSION).orig.tar.gz
DEBIAN_FILE = lldpd_$(LLDPD_VERSION_FULL).debian.tar.xz

DSC_FILE_URL = $(LLDP_URL)/$(DSC_FILE)
ORIG_FILE_URL = $(LLDP_URL)/$(ORIG_FILE)
DEBIAN_FILE_URL = $(LLDP_URL)/$(DEBIAN_FILE)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf lldpd-$(LLDPD_VERSION)

	# download debian LLDPDD
	dget $(DSC_FILE_URL)

	pushd lldpd-$(LLDPD_VERSION)
	git init
	git add -f *
	git commit -m "unmodified lldp source"

	# Apply patches
	stg init
	stg import -s ../patch/series

	# Build source and Debian packages
ifeq ($(CROSS_BUILD_ENVIRON), y)
	env "with_netlink_receive_bufsize=2*1024*1024" "with_netlink_max_receive_bufsize=4*1024*1024" CFLAGS="-I $$CROSS_PERL_CORE_PATH" dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	env "with_netlink_receive_bufsize=2*1024*1024" "with_netlink_max_receive_bufsize=4*1024*1024" dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	# Move the newly-built .deb packages to the destination directory
	mv $* $(DERIVED_TARGETS) $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

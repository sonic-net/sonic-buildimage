.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(NTP)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./ntp-$(NTP_VERSION) ./ntp_$(NTP_VERSION).orig.tar.xz ./ntp_$(NTP_VERSION)-4.debian.tar.xz

	# Get ntp release, debian files
	wget http://deb.debian.org/debian/pool/main/n/ntp/ntp_$(NTP_VERSION).orig.tar.xz
	wget http://deb.debian.org/debian/pool/main/n/ntp/ntp_$(NTP_VERSION)-1.debian.tar.xz

	# UnTar ntp release
	xzcat ntp_$(NTP_VERSION).orig.tar.xz | tar -xvf -

	pushd ./ntp-4.2.8p15

	# UnTar debian files
	xzcat ../ntp_$(NTP_VERSION)-1.debian.tar.xz | tar -xvf -

	# Add the additional patch
	cp ../patch/bug1970-UNLINK_EXPR_SLIST_empty_list.patch debian/patches/
	cp ../patch/update_ENOBUFS_log_level.patch debian/patches/
	cat ../patch/series  >> debian/patches/series

	# Update the changelog
	cat ../patch/changelog debian/changelog > debian/changelog.new
	rm debian/changelog ;  mv debian/changelog.new debian/changelog

	# The debian mirror build likely took place on a system without
	# libevent installed, thus adding the below for SONiC
	sed -i 's/--with-locfile=legacy/--with-locfile=legacy --enable-local-libevent/' debian/rules

	# Fix the apparmor profile to avoid the following message
	# "Failed name lookup - disconnected path"
	# and go into learning mode.
	sed -i 's/\/usr\/sbin\/ntpd {/\/usr\/sbin\/ntpd flags=(attach_disconnected complain) {/' debian/apparmor-profile

ifeq ($(CROSS_BUILD_ENVIRON), y)
	sed -i 's/dh_auto_configure \--/dh_auto_configure -- --with-yielding-select=yes /g' debian/rules
	echo ". $(CONFIG_SITE)" > fix.ntp.cross-config.$(CONFIGURED_ARCH)
	echo "unset with_openssl_libdir" >> fix.ntp.cross-config.$(CONFIGURED_ARCH)
	echo "unset with_openssl_incdir" >> fix.ntp.cross-config.$(CONFIGURED_ARCH)
	rm -f cross-config.cache
	ln -s /etc/dpkg-cross/cross-config.cache cross-config.cache
endif

	# Build source and Debian packages with the symbols
ifeq ($(CROSS_BUILD_ENVIRON), y)
	CONFIG_SITE=`pwd`/fix.ntp.cross-config.$(CONFIGURED_ARCH) dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	popd

	# Move the newly-built .deb packages to the destination directory
	mv $* $(DEST)/


.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(LIBPCRE3)
DERIVED_TARGETS = $(LIBPCRE3_DEV) $(LIBPCRE16_3) $(LIBPCRE32_3) $(LIBPCRECPP0V5) $(LIBPCRE3_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Obtaining pcre3
	rm -fr ./pcre3-$(LIBPCRE3_VERSION)
	dget https://packages.trafficmanager.net/public/debian/pool/main/p/pcre3_$(LIBPCRE3_VERSION)-$(LIBPCRE3_SUBVERSION).dsc
	pushd pcre3-$(LIBPCRE3_VERSION)

ifeq ($(CROSS_BUILD_ENVIRON), y)
	DPKG_GENSYMBOLS_CHECK_LEVEL=0 dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	DPKG_GENSYMBOLS_CHECK_LEVEL=0 dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

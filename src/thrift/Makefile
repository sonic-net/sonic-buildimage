SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e

THRIFT_VERSION = 0.11.0
THRIFT_VERSION_FULL = $(THRIFT_VERSION)-4

MAIN_TARGET = libthrift-$(THRIFT_VERSION)_$(THRIFT_VERSION_FULL)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = libthrift-dev_$(THRIFT_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  python-thrift_$(THRIFT_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  thrift-compiler_$(THRIFT_VERSION_FULL)_$(CONFIGURED_ARCH).deb

THRIFT_LINK_PRE = https://packages.trafficmanager.net/public/debian

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf thrift-$(THRIFT_VERSION)

	wget -NO "thrift_$(THRIFT_VERSION).orig.tar.gz" "$(THRIFT_LINK_PRE)/thrift_0.11.0.orig.tar.gz"
	wget -NO "thrift_$(THRIFT_VERSION_FULL).debian.tar.xz" "$(THRIFT_LINK_PRE)/thrift_0.11.0-4.debian.tar.xz"
	wget -NO "thrift_$(THRIFT_VERSION_FULL).dsc" "$(THRIFT_LINK_PRE)/thrift_0.11.0-4.dsc"

	dpkg-source -x thrift_$(THRIFT_VERSION_FULL).dsc
	pushd thrift-$(THRIFT_VERSION)

	# Revert breaking change in thrift 0.11.0
	# saithrift implementation relies on the bug in union serialization
	# (https://jira.apache.org/jira/browse/THRIFT-3650)
	patch -p1 < ../patch/0001-Revert-THRIFT-3650-incorrect-union-serialization.patch
	patch -p1 < ../patch/0002-cve-2017-1000487.patch
ifeq ($(CROSS_BUILD_ENVIRON), y)
	CC=$(CROSS_CC) LDSHARED=$(CROSS_CC) CXXFLAGS="-DFORCE_BOOST_SMART_PTR -DSIGNED_RIGHT_SHIFT_IS=1 -DARITHMETIC_RIGHT_SHIFT=1" CFLAGS="-DSIGNED_RIGHT_SHIFT_IS=1 -DARITHMETIC_RIGHT_SHIFT=1" dpkg-buildpackage -d -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	CXXFLAGS="-DFORCE_BOOST_SMART_PTR" DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -d -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

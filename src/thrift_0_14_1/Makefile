SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e -x

THRIFT_VERSION = 0.14.1

MAIN_TARGET = libthrift0_$(THRIFT_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = libthrift-dev_$(THRIFT_VERSION)_$(CONFIGURED_ARCH).deb \
		  python3-thrift_$(THRIFT_VERSION)_$(CONFIGURED_ARCH).deb \
		  python-thrift_$(THRIFT_VERSION)_$(CONFIGURED_ARCH).deb \
		  thrift-compiler_$(THRIFT_VERSION)_$(CONFIGURED_ARCH).deb

THRIFT_LINK_PRE = https://archive.apache.org/dist/thrift

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf thrift-$(THRIFT_VERSION)

	wget -O "thrift_$(THRIFT_VERSION).tar.gz" "$(THRIFT_LINK_PRE)/$(THRIFT_VERSION)/thrift-$(THRIFT_VERSION).tar.gz"

	tar -xvzf ./thrift_$(THRIFT_VERSION).tar.gz
	if [ -f thrift.patch/series ]; then pushd thrift-$(THRIFT_VERSION) && QUILT_PATCHES=../thrift.patch quilt push -a; [ -d .pc ] && rm -rf .pc; popd; fi

	pushd thrift-$(THRIFT_VERSION)
	DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -d -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

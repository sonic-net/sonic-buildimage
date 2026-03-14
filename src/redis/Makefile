SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e

REDIS_VERSION = 5.0.3
REDIS_VERSION_FULL = $(REDIS_VERSION)-3~bpo9+2

MAIN_TARGET = redis-server_$(REDIS_VERSION_FULL)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS = redis-tools_$(REDIS_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  redis-sentinel_$(REDIS_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  redis-tools-dbgsym_$(REDIS_VERSION_FULL)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf  redis_build
	mkdir redis_build
	pushd redis_build

	wget -O redis_$(REDIS_VERSION).orig.tar.gz -N "http://http.debian.net/debian/pool/main/r/redis/redis_$(REDIS_VERSION).orig.tar.gz"
	wget -O redis_$(REDIS_VERSION_FULL).dsc -N "http://http.debian.net/debian/pool/main/r/redis/redis_$(REDIS_VERSION_FULL).dsc"
	wget -O redis_$(REDIS_VERSION_FULL).debian.tar.xz -N "http://http.debian.net/debian/pool/main/r/redis/redis_$(REDIS_VERSION_FULL).debian.tar.xz"
	dpkg-source -x redis_$(REDIS_VERSION_FULL).dsc

	pushd redis-$(REDIS_VERSION)
	export ARCH=""
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -us -uc -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/
	popd

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

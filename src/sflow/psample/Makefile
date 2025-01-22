.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(PSAMPLE)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :

	rm -fr ./libpsample
	git clone https://github.com/Mellanox/libpsample.git
	cp -r debian libpsample

	pushd ./libpsample
	git checkout -b libpsample -f e48fad2

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $* $(DEST)/


.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = quicktrace_1.00_amd64.deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -fr ./quicktrace
	git clone https://github.com/aristanetworks/quicktrace-public quicktrace
	cp -r debian quicktrace

	pushd ./quicktrace/

ifeq ($(CROSS_BUILD_ENVIRON), y)
	CC=$(CROSS_CC) dpkg-buildpackage -rfakeroot -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif

	popd

	cp $* $(DEST)/


SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e

MAIN_TARGET = bash_$(BASH_VERSION_FULL)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Quilt store applied patches info in .pc folder, if this folder not clean, quilt can't apply patches correctly.
	rm -rf .pc
	rm -rf bash-$(BASH_VERSION_MAJOR)

	dget -u https://launchpad.net/debian/+archive/primary/+sourcefiles/bash/$(BASH_VERSION_FULL)/bash_$(BASH_VERSION_FULL).dsc

	# Apply plugin suport patches
	quilt push -a

	pushd bash-$(BASH_VERSION_MAJOR)

	# build package
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -us -uc -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)

	# copy UT code.
	cp -a ../Files/. ./

	# generate config.h which need by UT
	./configure

	# generate 'pathnames.h' by make bash again, which is needed by UT.
	make

	# run UT after config.h ready.
	make -C unittest && make test -C unittest
endif
	popd

	mv $* $(DEST)/

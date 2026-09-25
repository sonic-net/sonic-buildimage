.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = libcareplus_$(LIBCAREPLUS_VERSION_FULL)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./libcareplus ./libcareplus-$(LIBCAREPLUS_VERSION)

	# Get libcareplus release
	wget -O libcareplus-$(LIBCAREPLUS_VERSION).tar.gz $(LIBCAREPLUS_URL)
	tar -xzf libcareplus-$(LIBCAREPLUS_VERSION).tar.gz
	mv libcareplus-$(LIBCAREPLUS_VERSION) libcareplus
	pushd ./libcareplus

	# Create a git repository here so the patch series can be applied
	git init -q .
	git add -f .
	git -c user.name=sonic -c user.email=sonic@sonic.net commit -qm "initial commit"
	git -c user.name=sonic -c user.email=sonic@sonic.net am ../patch/*.patch

	# Build package
	ln -s ../debian debian
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
	popd

	mv $* $(DEST)/

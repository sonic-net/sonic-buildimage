.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = libnss-radius_$(NSS_RADIUS_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	pushd ./libnss-radius

	make clean
	-rm -rf debian
	-rm -rf patches
	cp -r ../debian .
	cp -r ../patches .

	# Apply patch (if any)

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -us -uc
else
	dpkg-buildpackage -rfakeroot -b -us -uc
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

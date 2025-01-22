.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = libpam-radius-auth_$(PAM_RADIUS_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Build freeradius-server v1.1.x
	pushd freeradius
	-rm -rf freeradius-server
	git clone https://github.com/FreeRADIUS/freeradius-server.git
	popd
	pushd freeradius/freeradius-server
	git checkout -f 5f715dba4d2dbdb268bf60fcc656352274930941
	# Apply patch
	patch -p1 < ../patches/freeradius_configure.patch
	patch -p1 < ../patches/freeradius_2007-04-06.patch
	patch -p1 < ../patches/freeradius_libeap_deprecated_openssl_1_0.patch
	patch -p1 < ../patches/libltdl_config_sub.patch
	cp ../patches/config.sub .
	cp ../patches/config.guess .
ifeq ($(CROSS_BUILD_ENVIRON), y)
	./configure --disable-static --enable-libtool-lock --host=$(CROSS_HOST_TYPE)
else
	./configure --disable-static --enable-libtool-lock
endif
	make
	popd

	# Obtain pam_radius
	-rm -rf ./pam_radius
	git clone https://github.com/FreeRADIUS/pam_radius.git
	pushd ./pam_radius
	# This is the latest commit with Radius MPL support
	git checkout -f 149c25df84cf5cd0e9addd9346699a9ca8fdddd2

	cp -r ../debian .

ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -rfakeroot -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -us -uc -nc
else
	dpkg-buildpackage -rfakeroot -b -us -uc -nc
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

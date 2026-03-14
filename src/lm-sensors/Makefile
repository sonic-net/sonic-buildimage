SHELL = /bin/bash
.ONESHELL:
	.SHELLFLAGS += -e


MAIN_TARGET = $(LM_SENSORS)
DERIVED_TARGETS = fancontrol_$(LM_SENSORS_VERSION_FULL)_all.deb \
				  $(LIBSENSORS) \
				  sensord_$(LM_SENSORS_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
				  $(LM_SENSORS_DBG) \
				  $(LIBSENSORS_DBG) \
				  $(SENSORD_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	rm -rf lm-sensors-$(LM_SENSORS_VERSION)
	dget -u http://deb.debian.org/debian/pool/main/l/lm-sensors/lm-sensors_$(LM_SENSORS_VERSION_FULL).dsc
	pushd lm-sensors-$(LM_SENSORS_VERSION)

	# Initialize as git repository
	git init
	git add -f *
	git commit -m "unmodified lm-sensors sources"

	# Apply patches
	stg init
	stg import -s ../patch/series

ifeq ($(CROSS_BUILD_ENVIRON), y)
	PROG_EXTRA=sensord dpkg-buildpackage -us -uc -b -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	DEB_BUILD_OPTIONS=nocheck PROG_EXTRA=sensord dpkg-buildpackage -us -uc -b -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

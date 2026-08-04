SHELL = /bin/bash
.ONESHELL:
.SHELLFLAGS += -e

MAIN_TARGET = $(BRCM_XLR_GTS_PLATFORM_MODULE)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Build the package
	export PYBUILD_INSTALL_ARGS_python2=--install-scripts=/dev/null
	dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)

	mv $(addprefix ../, $* $(EXTRA_TARGETS)) $(DEST)/

$(addprefix $(DEST)/, $(EXTRA_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

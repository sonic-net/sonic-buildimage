.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = mlnx-sai_1.mlnx.$(MLNX_SAI_VERSION)_$(CONFIGURED_ARCH).deb
DBGSYM_TARGET = mlnx-sai-dbgsym_1.mlnx.$(MLNX_SAI_VERSION)_$(CONFIGURED_ARCH).deb
DERIVED_TARGETS =
ifeq ($(findstring nostrip,$(DEB_BUILD_OPTIONS)),)
DERIVED_TARGETS += $(DBGSYM_TARGET)
endif

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# get sources
	rm -rf mlnx_sai
	wget -c $(MLNX_SAI_SOURCE_BASE_URL)/$(MLNX_SAI_VERSION).tar.gz -O - | tar -xz

	# build
	pushd mlnx_sai
	debuild -e 'make_extra_flags="DEFS=-DACS_OS -DCONFIG_SYSLOG -DCMAKE_SHARED_LINKER_FLAGS=-Wl,-Bsymbolic"' -us -uc -d -b
	popd

	mv $(DERIVED_TARGETS) $* $(DEST)/

ifneq ($(DERIVED_TARGETS),)
$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)
endif

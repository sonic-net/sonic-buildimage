.ONESHELL:
SHELL = /bin/bash

MAIN_TARGET = sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
DEV_TARGET = sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH)-dev.deb
DBGSYM_TARGET = sys-sdk_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH)-dbgsym.ddeb
DERIVED_TARGETS = $(DEV_TARGET)
ifeq ($(findstring nostrip,$(DEB_BUILD_OPTIONS)),)
DERIVED_TARGETS += $(DBGSYM_TARGET)
endif

PACKAGE_NAME = sys_sdk

# The SDK CLI assets are published next to SOURCES/, so their base URL is the
# source URL without that suffix. If the source URL does not end in /SOURCES,
# set MLNX_SDK_RELEASE_BASE_URL on the make command line.
MLNX_SDK_RELEASE_BASE_URL ?= $(patsubst %/SOURCES,%,$(MLNX_SDK_SOURCE_BASE_URL))

# Patch drops name the wheel without the dash in the SDK version, so
# 4.10.2026-001 ships sdkcli-4.10.2026001-wheel.tar.gz. Drops before
# 4.10.1984-002 do not reliably publish all of SDKCLI_ASSETS.
SDKCLI_WHEEL = sdkcli-$(subst -,,$(MLNX_SDK_VERSION))-wheel.tar.gz
SDKCLI_ASSETS = $(SDKCLI_WHEEL) $(SDKCLI_WHEEL).sha256sum install.py uninstall_sdkcli.sh
SDKCLI_BUNDLE_DIR = $(CURDIR)/builds/sdk_cli

# slave.mk exports DEB_BUILD_OPTIONS when SONIC_PROFILING_ON=y (nostrip noopt)
# or SONIC_DEBUGGING_ON=y (nostrip). CPack is not debhelper, so honor the same
# flags here. CMAKE_BUILD_TYPE is a configure-time compile setting.
# CPACK_DEBIAN_DEBUGINFO_PACKAGE is applied at cpack time.
# Default remains RelWithDebInfo + split dbgsym (production).
DEB_BUILD_OPTIONS ?=
SDK_CMAKE_ARGS := \
	-DPYTHON_INTERPRETERS=python3 \
	-DSKIP_UNINSTALL_INSTALL_DEPENDENCY=on \
	-DSKIP_DEPMOD_IN_INSTALL_PHASE=on \
	-DUSE_API_TESTER=OFF \
	-DUSE_KERNEL=OFF \
	-DCMAKE_PROJECT_VERSION=$(MLNX_SDK_DEB_VERSION) \
	-DUSE_PYTHON_SDK_API=ON \
	-DSDKCLI_INSTALL_MODE=venv \
	-DSDKCLI_BUNDLE_DIR=$(SDKCLI_BUNDLE_DIR) \
	-DSDKCLI_PYTHON=python3
# CPack leaves the -dev control file without a Depends on the runtime package.
# The component ids come from CMakeLists.txt: cpack_add_component(dev) and
# CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "main". CPack resolves them to the
# generated package names, so nothing here spells out sys-sdk-main.
# CPack also leaves the runtime deb without a Depends on shared libraries
# (libc6, libnl, libxml2, etc.); SHLIBDEPS fills that from linked libraries.
# GENERATE_SHLIBS writes a shlibs file so other packages (SAI) can Depends
# on sys-sdk-main from their own dpkg-shlibdeps run.
# SHLIBS_POLICY must be >= rather than the CPack default of =. The SONAME is
# the ABI contract, and SAI is routinely kept while only the SDK is bumped:
# an = pin freezes the SDK version into the SAI control file at SAI build time,
# so a cached or prebuilt SAI refuses to install next to a newer SDK.
# Quote only >= : this expands inside a bash recipe, where a bare > would
# be a redirection. Make does not treat > as special in a := value.
# cpack wants -D <var>=<value> as two arguments; the glued -Dvar=value form
# that cmake accepts is silently ignored here.
SDK_CPACK_ARGS := \
	-D CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS=ON \
	-D CPACK_COMPONENT_DEV_DEPENDS=main \
	-D CPACK_DEBIAN_PACKAGE_SHLIBDEPS=ON \
	-D CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS=ON \
	-D CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY='>='
ifneq ($(findstring noopt,$(DEB_BUILD_OPTIONS)),)
SDK_CMAKE_ARGS += -DCMAKE_BUILD_TYPE=Debug
endif
ifneq ($(findstring nostrip,$(DEB_BUILD_OPTIONS)),)
SDK_CPACK_ARGS += -D CPACK_DEBIAN_DEBUGINFO_PACKAGE=OFF
endif

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	set -eo pipefail

	# Get SDK sources and the published CLI wheel bundle.
	rm -rf $(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION)
	rm -rf $(SDKCLI_BUNDLE_DIR)

	# The SDK CLI assets are published next to SOURCES/. The release URL is derived
	# by dropping that suffix, which does nothing for a source URL not ending in it.
	if [ -z "$(MLNX_SDK_RELEASE_BASE_URL)" ] || [ "$(MLNX_SDK_RELEASE_BASE_URL)" = "$(MLNX_SDK_SOURCE_BASE_URL)" ]; then
		echo "Error: cannot derive the SDK release URL from MLNX_SDK_SOURCE_BASE_URL='$(MLNX_SDK_SOURCE_BASE_URL)'." >&2
		echo "It must end in /SOURCES, or MLNX_SDK_RELEASE_BASE_URL must be set on the make command line." >&2
		exit 1
	fi

	@echo  'wget -c $(MLNX_SDK_SOURCE_BASE_URL)/$(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION).tar.gz'

	wget -c $(MLNX_SDK_SOURCE_BASE_URL)/$(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION).tar.gz -O - | tar -xz

	mkdir -p $(SDKCLI_BUNDLE_DIR)
	for asset in $(SDKCLI_ASSETS); do
		url="$(MLNX_SDK_RELEASE_BASE_URL)/sdk_cli/$$asset"
		if ! wget -c "$$url" -O "$(SDKCLI_BUNDLE_DIR)/$$asset"; then
			echo "Error: cannot download the SDK CLI asset $$url" >&2
			echo "SDK $(MLNX_SDK_VERSION) may not publish it, or the release layout or file naming may have changed." >&2
			echo "The expected names and location are set by SDKCLI_ASSETS in $(CURDIR)/Makefile." >&2
			exit 1
		fi
	done
	(cd "$(SDKCLI_BUNDLE_DIR)" && sha256sum --check "$(SDKCLI_WHEEL).sha256sum")

	# Build the SDK
	rm -rf builds/cmake

	KVERSION=$$(ls -t /lib/modules 2>/dev/null | head -1 || true)

	# Debian Trixie build containers might not have /lib/modules entries/kernel headers.
	# Skip optional WJH eBPF build in such cases; SONiC does not use this feature.
	if [ -z "$$KVERSION" ] || [ ! -f "/lib/modules/$$KVERSION/source/include/linux/kconfig.h" ]; then
		if [ -f "wjh_libs/CMakeLists.txt" ]; then
			echo "Kernel headers are unavailable (KVERSION='$$KVERSION'), disabling WJH eBPF build"
			sed -i '/^[[:space:]]*if (WJH_EBPF_PRESENT)/,/^[[:space:]]*endif()/d' wjh_libs/CMakeLists.txt
		fi
	fi

	cmake -B builds/cmake $(SDK_CMAKE_ARGS) -DCMAKE_SYSTEM_VERSION=$$KVERSION
	cmake --build builds/cmake -j$(SONIC_CONFIG_MAKE_JOBS)

	cd builds/cmake

	cpack -G DEB $(SDK_CPACK_ARGS)

	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

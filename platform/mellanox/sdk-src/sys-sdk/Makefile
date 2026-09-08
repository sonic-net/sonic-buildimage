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
	-DCMAKE_PROJECT_VERSION=$(MLNX_SDK_DEB_VERSION)
# CPack leaves the -dev control file without a Depends on the runtime package.
# The component ids come from CMakeLists.txt: cpack_add_component(dev) and
# CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "main". CPack resolves them to the
# generated package names, so nothing here spells out sys-sdk-main.
# CPack also leaves the runtime deb without a Depends on shared libraries
# (libc6, libnl, libxml2, etc.); SHLIBDEPS fills that from linked libraries.
# GENERATE_SHLIBS writes a shlibs file so other packages (SAI) can Depends
# on sys-sdk-main from their own dpkg-shlibdeps run.
# cpack wants -D <var>=<value> as two arguments; the glued -Dvar=value form
# that cmake accepts is silently ignored here.
SDK_CPACK_ARGS := \
	-D CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS=ON \
	-D CPACK_COMPONENT_DEV_DEPENDS=main \
	-D CPACK_DEBIAN_PACKAGE_SHLIBDEPS=ON \
	-D CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS=ON
ifneq ($(findstring noopt,$(DEB_BUILD_OPTIONS)),)
SDK_CMAKE_ARGS += -DCMAKE_BUILD_TYPE=Debug
endif
ifneq ($(findstring nostrip,$(DEB_BUILD_OPTIONS)),)
SDK_CPACK_ARGS += -D CPACK_DEBIAN_DEBUGINFO_PACKAGE=OFF
endif

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# get sources
	rm -rf $(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION)
	wget -c $(MLNX_SDK_SOURCE_BASE_URL)/$(PACKAGE_NAME)-$(MLNX_SDK_VERSION)-$(MLNX_SDK_ISSU_VERSION).tar.gz -O - | tar -xz

	# build
	rm -rf builds/cmake

	KVERSION=$$(ls -t /lib/modules 2>/dev/null | head -1)

	# Debian Trixie build containers might not have /lib/modules entries/kernel headers.
	# Skip optional WJH eBPF build in such cases sonic doesn't use this feature.
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

# linux kernel package

KVERSION_SHORT = 6.1.0-29-2
KVERSION = $(KVERSION_SHORT)-$(CONFIGURED_ARCH)
KERNEL_VERSION = 6.1.123
KERNEL_SUBVERSION = 1
ifeq ($(CONFIGURED_ARCH), armhf)
# Override kernel version for ARMHF as it uses arm MP (multi-platform) for short version
KVERSION = $(KVERSION_SHORT)-armmp
endif

# Place an URL here to .tar.gz file if you want to include those patches
EXTERNAL_KERNEL_PATCH_URL =
# Set y to include non upstream patches tarball provided by the corresponding platform
INCLUDE_EXTERNAL_PATCHES ?= n
# platforms should override this and provide an absolute location to the patches
EXTERNAL_KERNEL_PATCH_LOC =

export KVERSION_SHORT KVERSION KERNEL_VERSION KERNEL_SUBVERSION
export EXTERNAL_KERNEL_PATCH_URL
export INCLUDE_EXTERNAL_PATCHES
export EXTERNAL_KERNEL_PATCH_LOC

LINUX_HEADERS_COMMON = linux-headers-$(KVERSION_SHORT)-common_$(KERNEL_VERSION)-$(KERNEL_SUBVERSION)_all.deb
$(LINUX_HEADERS_COMMON)_SRC_PATH = $(SRC_PATH)/sonic-linux-kernel
$(LINUX_HEADERS_COMMON)_CACHE_OVERRIDE = $(SONIC_DPKG_CACHE_METHOD_OVERRIDE)
SONIC_MAKE_DEBS += $(LINUX_HEADERS_COMMON)

LINUX_HEADERS = linux-headers-$(KVERSION)_$(KERNEL_VERSION)-$(KERNEL_SUBVERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LINUX_HEADERS_COMMON),$(LINUX_HEADERS)))

ifeq ($(CONFIGURED_ARCH), armhf)
	LINUX_KERNEL = linux-image-$(KVERSION)_$(KERNEL_VERSION)-$(KERNEL_SUBVERSION)_$(CONFIGURED_ARCH).deb
else
	LINUX_KERNEL = linux-image-$(KVERSION)-unsigned_$(KERNEL_VERSION)-$(KERNEL_SUBVERSION)_$(CONFIGURED_ARCH).deb
endif
$(eval $(call add_derived_package,$(LINUX_HEADERS_COMMON),$(LINUX_KERNEL)))

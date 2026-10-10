# libcareplus package
#
# Userspace live-patching engine behind the func_hotpatch patch type. The
# device needs libcare-ctl to apply patches at runtime; the build host needs
# libcare-patch-make and friends to produce .kpatch objects.

LIBCAREPLUS_VERSION = 1.0.2
LIBCAREPLUS_VERSION_FULL = $(LIBCAREPLUS_VERSION)-1
LIBCAREPLUS_TAG = v$(LIBCAREPLUS_VERSION)
LIBCAREPLUS_URL = https://github.com/openeuler-mirror/libcareplus/archive/refs/tags/$(LIBCAREPLUS_TAG).tar.gz

export LIBCAREPLUS_VERSION LIBCAREPLUS_VERSION_FULL LIBCAREPLUS_TAG LIBCAREPLUS_URL

LIBCAREPLUS = libcareplus_$(LIBCAREPLUS_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(LIBCAREPLUS)_SRC_PATH = $(SRC_PATH)/sonic-hotpatch/libcareplus
SONIC_MAKE_DEBS += $(LIBCAREPLUS)

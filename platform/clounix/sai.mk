SDK_VERSION = 1.11.0
VENDOR_FEATURE_VERSION=3
VENDOR_PATCH_VERSION=2304
VENDOR_REL_TYPE=CU
# Place here URL where SAI deb exist
#
#
ifeq ($(SAI_DEB_URL),no)
SAI_SOURCE_PATH=$(PLATFORM_PATH)/clx-sai
VENDOR_VERSION=$(shell $(SAI_SOURCE_PATH)/version/autogen.sh $(SAI_HEADER_VERSION) $(SAI_SOURCE_PATH))
SAI_VENDOR_VERSION = $(SAI_HEADER_VERSION).$(VENDOR_VERSION)

CLOUNIX_SAI = libsaiclx_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb
$(CLOUNIX_SAI)_SRC_PATH = $(PLATFORM_PATH)/clx-sai
$(CLOUNIX_SAI)_DEPENDS += $(LIBCAREPLUS)

ifeq ($(INCLUDE_SDK),yes)
$(CLOUNIX_SAI)_UNINSTALLS += $(CLOUNIX_SDK)
else
SONIC_DPKG_DEBS += $(CLOUNIX_SAI)
endif

CLOUNIX_SAI_DEV = libsaiclx-dev_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb
$(CLOUNIX_SAI_DEV)_DEPENDS += $(CLOUNIX_SAI)

CLOUNIX_SAI_DBG = libsaiclx-dbg_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb
$(CLOUNIX_SAI_DBG)_DEPENDS += $(CLOUNIX_SAI)
else
VENDOR_VERSION=$(VENDOR_FEATURE_VERSION).$(VENDOR_REL_TYPE)
SAI_VENDOR_VERSION = $(SAI_HEADER_VERSION).$(VENDOR_VERSION)
CLOUNIX_SAI = libsaiclx_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb
$(CLOUNIX_SAI)_URL = "https://github.com/clounix/sai_release/raw/main/sai_available/libsaiclx_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb"

CLOUNIX_SAI_DEV = libsaiclx-dev_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb
$(CLOUNIX_SAI_DEV)_URL = "https://github.com/clounix/sai_release/raw/main/sai_available/libsaiclx-dev_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb"

CLOUNIX_SAI_DBG = libsaiclx-dbg_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb
$(CLOUNIX_SAI_DBG)_URL = "https://github.com/clounix/sai_release/raw/main/sai_available/libsaiclx-dbg_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb"

SONIC_ONLINE_DEBS += $(CLOUNIX_SAI)
endif

ifneq ($(INCLUDE_SDK),yes)
$(eval $(call add_derived_package,$(CLOUNIX_SAI),$(CLOUNIX_SAI_DEV)))
$(eval $(call add_derived_package,$(CLOUNIX_SAI),$(CLOUNIX_SAI_DBG)))
$(eval $(call add_conflict_package,$(CLOUNIX_SAI_DEV),$(LIBSAIVS_DEV)))
endif

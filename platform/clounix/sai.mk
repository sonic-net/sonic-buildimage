SDK_VERSION=2.1.0
SAI_DATE=250513
SAI_CU=HE
VENDOR_FEATURE_VERSION=2.1
# Place here URL where SAI deb exist
#
SAI_DEB_URL ?= yes
SAI_HEADER_VERSION ?= 1.14.0
ifeq ($(SAI_DEB_URL),no)
SAI_SOURCE_PATH=$(PLATFORM_PATH)/clx-sai
SAI_VENDOR_VERSION=$(strip $(shell $(SAI_SOURCE_PATH)/version/autogen.sh $(SAI_HEADER_VERSION) $(SAI_SOURCE_PATH)))

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
SAI_VENDOR_VERSION = ${VENDOR_FEATURE_VERSION}-$(SAI_HEADER_VERSION)+$(SAI_DATE).$(SAI_CU)
CLOUNIX_SAI = libsaiclx_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb
$(CLOUNIX_SAI)_PATH = $(PLATFORM_PATH)/clx-sai
$(CLOUNIX_SSAI_VENDOR_VERSIONAI)_URL = "https://github.com/clounix/sai_release/blob/main/libsaiclx-$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb"

CLOUNIX_SAI_DEV = libsaiclx-dev_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb
$(CLOUNIX_SAI_DEV)_PATH = $(PLATFORM_PATH)/clx-sai
$(CLOUNIX_SAI)_URL = "https://github.com/clounix/sai_release/blob/main/libsaiclx-dev_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb"

CLOUNIX_SAI_DBG = libsaiclx-dbg_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb
$(CLOUNIX_SAI_DBG)_PATH = $(PLATFORM_PATH)/clx-sai
$(CLOUNIX_SAI)_URL = "https://github.com/clounix/sai_release/blob/main/libsaiclx-dbg_$(SAI_VENDOR_VERSION)_$(PLATFORM_ARCH).deb"

#SONIC_ONLINE_DEBS += $(CLOUNIX_SAI)
SONIC_COPY_DEBS += $(CLOUNIX_SAI)
endif

ifneq ($(INCLUDE_SDK),yes)
$(eval $(call add_derived_package,$(CLOUNIX_SAI),$(CLOUNIX_SAI_DEV)))
$(eval $(call add_derived_package,$(CLOUNIX_SAI),$(CLOUNIX_SAI_DBG)))
$(eval $(call add_conflict_package,$(CLOUNIX_SAI_DEV),$(LIBSAIVS_DEV)))
endif

export clx_sai_deb = $(CLOUNIX_SAI)
export clx_sai_dev_deb = $(CLOUNIX_SAI_DEV)

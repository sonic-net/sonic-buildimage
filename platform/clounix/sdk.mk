SDK_VERSION = 2.0.0

# Place here URL where SAI deb exist
#
CLOUNIX_SDK = libclxsdk_$(SDK_VERSION)_amd64.deb
$(CLOUNIX_SDK)_SRC_PATH = $(PLATFORM_PATH)/clounix-sdk
$(CLOUNIX_SDK)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
SONIC_DPKG_DEBS += $(CLOUNIX_SDK)

CLOUNIX_SDK_DEV = libclxsdk-dev_$(SDK_VERSION)_amd64.deb
$(CLOUNIX_SDK_DEV)_DEPENDS += $(CLOUNIX_SDK)

CLOUNIX_SDK_DBG = libclxsdk-dbg_$(SDK_VERSION)_amd64.deb
$(CLOUNIX_SDK_DBG)_DEPENDS += $(CLOUNIX_SDK)

$(eval $(call add_derived_package,$(CLOUNIX_SDK),$(CLOUNIX_SDK_DEV)))
$(eval $(call add_derived_package,$(CLOUNIX_SDK),$(CLOUNIX_SDK_DBG)))

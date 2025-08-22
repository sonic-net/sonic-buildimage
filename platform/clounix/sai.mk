SDK_VERSION = 3.0.0
SAI_HEADER_VERSION = 1.7.3.0-1.RC1
#SAI_CHIP_ID = dawn
SAI_CHIP_ID = lightning
export SAI_HEADER_VERSION SAI_CHIP_ID

# Place here URL where SAI deb exist
#
SAI_DEB_URL = yes
ifeq ($(SAI_DEB_URL),no)
CLOUNIX_SAI = libsaiclx_$(SAI_HEADER_VERSION)_amd64.deb
$(CLOUNIX_SAI)_SRC_PATH = $(PLATFORM_PATH)/clnx-sai
$(CLOUNIX_SAI)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
SONIC_DPKG_DEBS += $(CLOUNIX_SAI)

CLOUNIX_SAI_DEV = libsaiclx-dev_$(SAI_HEADER_VERSION)_amd64.deb
$(CLOUNIX_SAI_DEV)_DEPENDS += $(CLOUNIX_SAI)

CLOUNIX_WARM_VERIFIER = clx-app_$(SAI_HEADER_VERSION)_amd64.deb
$(CLOUNIX_WARM_VERIFIER)_DEPENDS += $(CLOUNIX_SAI)

CLOUNIX_SAI_DBG = libsaiclx-dbg_$(SAI_HEADER_VERSION)_amd64.deb
$(CLOUNIX_SAI_DBG)_DEPENDS += $(CLOUNIX_SAI)
$(eval $(call add_derived_package,$(CLOUNIX_SAI),$(CLOUNIX_SAI_DEV)))
$(eval $(call add_derived_package,$(CLOUNIX_SAI),$(CLOUNIX_WARM_VERIFIER)))
$(eval $(call add_derived_package,$(CLOUNIX_SAI),$(CLOUNIX_SAI_DBG)))	
else
CLOUNIX_SAI = libsaiclx_$(SAI_HEADER_VERSION)_amd64.deb
$(CLOUNIX_SAI)_PATH = $(PLATFORM_PATH)/clx-sai
$(CLOUNIX_SAI)_URL = "https://github.com/clounix/sai_release/raw/main/sai_available/libsaiclx_$(SAI_HEADER_VERSION)_amd64.deb"

CLOUNIX_SAI_DEV = libsaiclx-dev_$(SAI_HEADER_VERSION)_amd64.deb
$(CLOUNIX_SAI_DEV)_PATH = $(PLATFORM_PATH)/clx-sai
$(CLOUNIX_SAI_DEV)_URL = "https://github.com/clounix/sai_release/raw/main/sai_available/libsaiclx-dev_$(SAI_HEADER_VERSION)_amd64.deb"

CLOUNIX_SAI_DBG = libsaiclx-dbg_$(SAI_HEADER_VERSION)_amd64.deb
$(CLOUNIX_SAI_DBG)_PATH = $(PLATFORM_PATH)/clx-sai
$(CLOUNIX_SAI_DBG)_URL = "https://github.com/clounix/sai_release/raw/main/sai_available/libsaiclx-dbg_$(SAI_HEADER_VERSION)_amd64.deb"
#SONIC_COPY_DEBS += $(CLOUNIX_SAI) $(CLOUNIX_SAI_DEV)
SONIC_ONLINE_DEBS+= $(CLOUNIX_SAI) $(CLOUNIX_SAI_DEV)
endif


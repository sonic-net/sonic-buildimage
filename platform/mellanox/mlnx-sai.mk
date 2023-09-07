# Mellanox SAI

MLNX_SAI_VERSION = SAIBuild2211.25.1.4
MLNX_SAI_ASSETS_GITHUB_URL = https://github.com/Mellanox/Spectrum-SDK-Drivers-SONiC-Bins
MLNX_SAI_ASSETS_RELEASE_TAG = sai-$(MLNX_SAI_VERSION)-$(BLDENV)-$(CONFIGURED_ARCH)
MLNX_SAI_ASSETS_URL = $(MLNX_ASSETS_GITHUB_URL)/releases/download/$(MLNX_SAI_ASSETS_RELEASE_TAG)
MLNX_SAI_DEB_VERSION = $(subst -,.,$(subst _,.,$(MLNX_SAI_VERSION)))

# Place here URL where SAI sources exist
MLNX_SAI_SOURCE_BASE_URL = 

ifneq ($(MLNX_SAI_SOURCE_BASE_URL), )
SAI_FROM_SRC = y
else
SAI_FROM_SRC = n
endif

export MLNX_SAI_VERSION MLNX_SAI_SOURCE_BASE_URL

MLNX_SAI = mlnx-sai_1.mlnx.$(MLNX_SAI_VERSION)_$(CONFIGURED_ARCH).deb
$(MLNX_SAI)_SRC_PATH = $(PLATFORM_PATH)/mlnx-sai
$(MLNX_SAI)_DEPENDS += $(MLNX_SDK_DEBS)
$(MLNX_SAI)_RDEPENDS += $(MLNX_SDK_RDEBS) $(MLNX_SDK_DEBS)
$(eval $(call add_conflict_package,$(MLNX_SAI),$(LIBSAIVS_DEV)))
MLNX_SAI_DBGSYM = mlnx-sai-dbgsym_1.mlnx.$(MLNX_SAI_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(MLNX_SAI),$(MLNX_SAI_DBGSYM)))

define make_url
	$(1)_URL = $(MLNX_SAI_ASSETS_URL)/$(1)

endef

$(eval $(foreach deb,$(MLNX_SAI) $(MLNX_SAI_DBGSYM),$(call make_url,$(deb))))

ifeq ($(SAI_FROM_SRC), y)
SONIC_MAKE_DEBS += $(MLNX_SAI)
else
SONIC_ONLINE_DEBS += $(MLNX_SAI)
endif

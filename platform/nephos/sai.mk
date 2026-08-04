SDK_VERSION = 3.0.0
SAI_VERSION = 1.5.0
SAI_COMMIT_ID = 06a67d

# Place here URL where SAI deb exist
NEPHOS_SAI_DEB_LOCAL_URL =
export NEPHOS_SAI_DEB_LOCAL_URL
#
ifneq ($(NEPHOS_SAI_DEB_LOCAL_URL), )
SAI_FROM_LOCAL = y
else
SAI_FROM_LOCAL = n
endif

NEPHOS_SAI = libsainps_$(SDK_VERSION)_sai_$(SAI_VERSION)_$(SAI_COMMIT_ID)_amd64.deb
ifeq ($(SAI_FROM_LOCAL), y)
$(NEPHOS_SAI)_PATH = $(NEPHOS_SAI_DEB_LOCAL_URL)
else
$(NEPHOS_SAI)_URL = "https://github.com/NephosInc/SONiC/raw/master/sai/libsainps_$(SDK_VERSION)_sai_$(SAI_VERSION)_$(SAI_COMMIT_ID)_amd64.deb"
endif

NEPHOS_SAI_DEV = libsainps-dev_$(SDK_VERSION)_sai_$(SAI_VERSION)_$(SAI_COMMIT_ID)_amd64.deb
$(eval $(call add_derived_package,$(NEPHOS_SAI),$(NEPHOS_SAI_DEV)))
ifeq ($(SAI_FROM_LOCAL), y)
$(NEPHOS_SAI_DEV)_PATH = $(NEPHOS_SAI_DEB_LOCAL_URL)
else
$(NEPHOS_SAI_DEV)_URL = "https://github.com/NephosInc/SONiC/raw/master/sai/libsainps-dev_$(SDK_VERSION)_sai_$(SAI_VERSION)_$(SAI_COMMIT_ID)_amd64.deb"
endif

ifeq ($(SAI_FROM_LOCAL), y)
SONIC_COPY_DEBS += $(NEPHOS_SAI) $(NEPHOS_SAI_DEV)
else
SONIC_ONLINE_DEBS += $(NEPHOS_SAI) $(NEPHOS_SAI_DEV)
endif
$(NEPHOS_SAI_DEV)_DEPENDS += $(NEPHOS_SAI)
$(eval $(call add_conflict_package,$(NEPHOS_SAI_DEV),$(LIBSAIVS_DEV)))

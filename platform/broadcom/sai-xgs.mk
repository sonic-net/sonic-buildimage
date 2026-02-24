# Broadcom XGS SAI definitions
LIBSAIBCM_XGS_VERSION = 14.1.0.1.0.0.9.0
LIBSAIBCM_XGS_BRANCH_NAME = SAI_14.1.0_GA

LIBSAIBCM_XGS_URL_PREFIX = "$(BUILD_PUBLIC_URL)/sai/sai-broadcom/$(LIBSAIBCM_XGS_BRANCH_NAME)/$(LIBSAIBCM_XGS_VERSION)/xgs"

# Runtime package
BRCM_XGS_SAI = libsaibcm_$(LIBSAIBCM_XGS_VERSION)_amd64.deb
$(BRCM_XGS_SAI)_URL = "$(LIBSAIBCM_XGS_URL_PREFIX)/$(BRCM_XGS_SAI)"

# Development package
BRCM_XGS_SAI_DEV = libsaibcm-dev_$(LIBSAIBCM_XGS_VERSION)_amd64.deb
$(eval $(call add_derived_package,$(BRCM_XGS_SAI),$(BRCM_XGS_SAI_DEV)))
$(BRCM_XGS_SAI_DEV)_URL = "$(LIBSAIBCM_XGS_URL_PREFIX)/$(BRCM_XGS_SAI_DEV)"

# Package registration
SONIC_ONLINE_DEBS += $(BRCM_XGS_SAI)

# Dependencies
$(BRCM_XGS_SAI_DEV)_DEPENDS += $(BRCM_XGS_SAI)

# Version handling
$(BRCM_XGS_SAI)_SKIP_VERSION=y
$(BRCM_XGS_SAI_DEV)_SKIP_VERSION=y

# Conflicts
$(eval $(call add_conflict_package,$(BRCM_XGS_SAI_DEV),$(LIBSAIVS_DEV)))

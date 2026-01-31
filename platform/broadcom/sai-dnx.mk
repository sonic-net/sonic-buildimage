# Broadcom DNX SAI definitions
LIBSAIBCM_DNX_VERSION = 14.1.0.1.0.0.5.0
LIBSAIBCM_DNX_BRANCH_NAME = SAI_14.1.0_GA

LIBSAIBCM_DNX_URL_PREFIX = "https://packages.trafficmanager.net/public/sai/sai-broadcom/$(LIBSAIBCM_DNX_BRANCH_NAME)/$(LIBSAIBCM_DNX_VERSION)/dnx"

# SAI module for DNX Asic family
BRCM_DNX_SAI = libsaibcm_dnx_$(LIBSAIBCM_DNX_VERSION)_amd64.deb
$(BRCM_DNX_SAI)_URL = "$(LIBSAIBCM_DNX_URL_PREFIX)/$(BRCM_DNX_SAI)"

# Package registration
SONIC_ONLINE_DEBS += $(BRCM_DNX_SAI)

# Version handling
$(BRCM_DNX_SAI)_SKIP_VERSION=y

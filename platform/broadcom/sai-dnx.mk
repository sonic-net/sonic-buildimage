# Broadcom DNX SAI definitions
LIBSAIBCM_DNX_VERSION = 14.1.0.1.0.0.13.0
LIBSAIBCM_DNX_BRANCH_NAME = dev-dnx
LIBSAIBCM_DNX_URL_PREFIX = "http://sonic-pluto.wfr.ion.nokia.net/files/sonic/build/sai/$(LIBSAIBCM_DNX_BRANCH_NAME)/1363405/dnx"


# SAI module for DNX Asic family
BRCM_DNX_SAI = libsaibcm_dnx_$(LIBSAIBCM_DNX_VERSION)_amd64.deb
$(BRCM_DNX_SAI)_URL = "$(LIBSAIBCM_DNX_URL_PREFIX)/$(BRCM_DNX_SAI)"

# Package registration
SONIC_ONLINE_DEBS += $(BRCM_DNX_SAI)

# Version handling
$(BRCM_DNX_SAI)_SKIP_VERSION=y

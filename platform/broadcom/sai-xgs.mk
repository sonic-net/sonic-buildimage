# Broadcom XGS SAI definitions
#
# SAI 15.2 is built against Debian trixie (libc6 >= 2.38, libstdc++6 >= 14,
# libprotobuf32t64) and cannot be installed in the bookworm slave. The final
# image is trixie-based (IMAGE_DISTRO=trixie), so production artifacts use
# 15.2. The bookworm pass keeps building SAI 14.3 for transitional artifacts
# (e.g. the syncd binary baked into gearbox containers) until broadcom is
# fully migrated to trixie.
ifeq ($(BLDENV), trixie)
LIBSAIBCM_XGS_VERSION = 15.2.0.0.0.0.0.0
LIBSAIBCM_XGS_BRANCH_NAME = SAI_15.2.0_GA
else
LIBSAIBCM_XGS_VERSION = 14.3.0.0.0.0.14.0
LIBSAIBCM_XGS_BRANCH_NAME = SAI_14.3.0_GA
endif

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

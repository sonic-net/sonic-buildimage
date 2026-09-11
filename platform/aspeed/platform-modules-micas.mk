# Micas Platform modules
#

# Common package - contains shared utilities for Micas Aspeed-based BMC cards
MICAS_COMMON_PLATFORM_MODULE = sonic-platform-aspeed-micas-common_1.0_arm64.deb
$(MICAS_COMMON_PLATFORM_MODULE)_SRC_PATH = $(PLATFORM_PATH)/sonic-platform-modules-micas
$(MICAS_COMMON_PLATFORM_MODULE)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
$(MICAS_COMMON_PLATFORM_MODULE)_PLATFORM = arm64-micas-common
SONIC_DPKG_DEBS += $(MICAS_COMMON_PLATFORM_MODULE)

# M2-W6950-128OC platform package
ASPEED_MICAS_M2_W6950_128OC_PLATFORM_MODULE = sonic-platform-aspeed-micas-m2-w6950-128oc_1.0_arm64.deb
$(ASPEED_MICAS_M2_W6950_128OC_PLATFORM_MODULE)_PLATFORM = arm64-micas_m2-w6950-128oc-r0
$(eval $(call add_extra_package,$(MICAS_COMMON_PLATFORM_MODULE),$(ASPEED_MICAS_M2_W6950_128OC_PLATFORM_MODULE)))

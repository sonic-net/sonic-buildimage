# NextHop Platform modules
#

# Common package - contains USB network gadget and shared utilities
NEXTHOP_COMMON_PLATFORM_MODULE = sonic-platform-aspeed-nexthop-common_1.0_arm64.deb
$(NEXTHOP_COMMON_PLATFORM_MODULE)_SRC_PATH = $(PLATFORM_PATH)/sonic-platform-modules-nexthop
$(NEXTHOP_COMMON_PLATFORM_MODULE)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
$(NEXTHOP_COMMON_PLATFORM_MODULE)_PLATFORM = arm64-nexthop-common
SONIC_DPKG_DEBS += $(NEXTHOP_COMMON_PLATFORM_MODULE)

# B27 platform package
ASPEED_NEXTHOP_B27_PLATFORM_MODULE = sonic-platform-aspeed-nexthop-b27_1.0_arm64.deb
$(ASPEED_NEXTHOP_B27_PLATFORM_MODULE)_PLATFORM = arm64-nexthop_b27-r0
$(eval $(call add_extra_package,$(NEXTHOP_COMMON_PLATFORM_MODULE),$(ASPEED_NEXTHOP_B27_PLATFORM_MODULE)))

# Future B28 platform (example):
# ASPEED_NEXTHOP_B28_PLATFORM_MODULE = sonic-platform-aspeed-nexthop-b28_1.0_arm64.deb
# $(ASPEED_NEXTHOP_B28_PLATFORM_MODULE)_PLATFORM = arm64-nexthop_b28-r0
# $(eval $(call add_extra_package,$(NEXTHOP_COMMON_PLATFORM_MODULE),$(ASPEED_NEXTHOP_B28_PLATFORM_MODULE)))


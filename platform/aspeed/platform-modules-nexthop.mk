# NextHop Platform modules
#
# NOTE: When adding more platforms (e.g., b28), use add_extra_package:
#   ASPEED_NEXTHOP_B28_PLATFORM_MODULE = sonic-platform-aspeed-nexthop-b28_1.0_arm64.deb
#   $(ASPEED_NEXTHOP_B28_PLATFORM_MODULE)_PLATFORM = arm64-nexthop_b28-r0
#   $(eval $(call add_extra_package,$(ASPEED_NEXTHOP_B27_PLATFORM_MODULE),$(ASPEED_NEXTHOP_B28_PLATFORM_MODULE)))

ASPEED_NEXTHOP_B27_PLATFORM_MODULE = sonic-platform-aspeed-nexthop-b27_1.0_arm64.deb
$(ASPEED_NEXTHOP_B27_PLATFORM_MODULE)_SRC_PATH = $(PLATFORM_PATH)/sonic-platform-modules-nexthop
$(ASPEED_NEXTHOP_B27_PLATFORM_MODULE)_DEPENDS += $(ASPEED_COMMON_PLATFORM_MODULE)
$(ASPEED_NEXTHOP_B27_PLATFORM_MODULE)_PLATFORM = arm64-nexthop_b27-r0
SONIC_DPKG_DEBS += $(ASPEED_NEXTHOP_B27_PLATFORM_MODULE)

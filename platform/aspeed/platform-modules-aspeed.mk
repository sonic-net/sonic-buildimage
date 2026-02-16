# SONiC Aspeed Vendor Platform Packages
# This builds vendor-specific packages from one source directory
# Note: The common base package is installed directly into the pmon Docker image,
#       not built as a separate debian package

# EVB vendor package
ASPEED_VENDOR_EVB = sonic-bmc-platform-aspeed-evb_1.0.0_arm64.deb
$(ASPEED_VENDOR_EVB)_SRC_PATH = $(PLATFORM_PATH)/vendor
$(ASPEED_VENDOR_EVB)_DEPENDS =
$(ASPEED_VENDOR_EVB)_RDEPENDS =
SONIC_DPKG_DEBS += $(ASPEED_VENDOR_EVB)

# NextHop vendor package
ASPEED_VENDOR_NEXTHOP = sonic-bmc-platform-aspeed-nexthop_1.0.0_arm64.deb
$(ASPEED_VENDOR_NEXTHOP)_SRC_PATH = $(PLATFORM_PATH)/vendor
$(ASPEED_VENDOR_NEXTHOP)_DEPENDS =
$(ASPEED_VENDOR_NEXTHOP)_RDEPENDS =
SONIC_DPKG_DEBS += $(ASPEED_VENDOR_NEXTHOP)

# Export for use in image builds
export ASPEED_VENDOR_EVB
export ASPEED_VENDOR_NEXTHOP


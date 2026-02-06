# SONiC BMC Platform Package for Aspeed AST2700

BMC_PLATFORM_ASPEED = sonic-bmc-platform-aspeed_1.0.0_arm64.deb
$(BMC_PLATFORM_ASPEED)_SRC_PATH = $(PLATFORM_PATH)/sonic-bmc-platform-aspeed
$(BMC_PLATFORM_ASPEED)_DEPENDS =
$(BMC_PLATFORM_ASPEED)_RDEPENDS =
SONIC_DPKG_DEBS += $(BMC_PLATFORM_ASPEED)

# Export for use in image builds
export BMC_PLATFORM_ASPEED


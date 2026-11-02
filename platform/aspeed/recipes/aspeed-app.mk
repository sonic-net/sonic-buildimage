# Integrate Aspeed application tools (like OTP programming utility):
# https://github.com/AspeedTech-BMC/aspeed_app
#
# Source-only build: the recipe clones the upstream git repository at the
# pinned tag and builds the .deb locally (see aspeed-app/Makefile).

ASPEED_APP_VERSION ?= 00.01.23

ASPEED_APP_PKG_RELEASE ?= 1

ASPEED_APP_PKG_VERSION = $(ASPEED_APP_VERSION)-$(ASPEED_APP_PKG_RELEASE)

ASPEED_APP = aspeed-app_$(ASPEED_APP_PKG_VERSION)_$(CONFIGURED_ARCH).deb
$(ASPEED_APP)_SRC_PATH = $(PLATFORM_PATH)/aspeed-app

export ASPEED_APP_VERSION ASPEED_APP_PKG_VERSION ASPEED_APP

SONIC_MAKE_DEBS += $(ASPEED_APP)

ASPEED_APP_PACKAGES = $(ASPEED_APP)
aspeed-app-packages: $(addprefix $(DEBS_PATH)/, $(ASPEED_APP_PACKAGES))

SONIC_PHONY_TARGETS += aspeed-app-packages

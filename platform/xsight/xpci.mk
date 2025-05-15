# Xsight xpci

XPCI_VERSION = 1.0

export XPCI_VERSION

XPCI = xpci-modules-$(KVERSION)_$(XPCI_VERSION)_amd64.deb

$(XPCI)_SRC_PATH = $(PLATFORM_PATH)/xpci
SONIC_MAKE_DEBS += $(XPCI)


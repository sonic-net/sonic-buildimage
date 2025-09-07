# Xsight xpci

XPCI_VERSION = 0.7.0

export XPCI_VERSION

XSIGHT_XPCI_URL_PREFIX = "https://raw.githubusercontent.com/xsightlabs/sonic-xsight-binaries/main/amd64/kernel/"
XPCI = xpci-dkms_$(XPCI_VERSION)_amd64.deb
$(XPCI)_URL = "$(XSIGHT_XPCI_URL_PREFIX)/$(XPCI)"
SONIC_ONLINE_DEBS += $(XPCI)

KERNEL_XPCI = xpci-modules-$(KVERSION)_$(XPCI_VERSION)_amd64.deb
$(KERNEL_XPCI)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON) $(XPCI)

$(KERNEL_XPCI)_SRC_PATH = $(PLATFORM_PATH)/xpci
SONIC_MAKE_DEBS += $(KERNEL_XPCI)

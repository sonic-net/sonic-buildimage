.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

XPCI_DEB = xpci-modules-$(KVERSION)_$(XPCI_VERSION)_amd64.deb
XPCI_URL_PREFIX = "https://github.com/Xsight-Labs/SONiC/raw/refs/heads/main/amd64/kernel"
REPACKED_XPCI_NAME = xpci-$(XPCI_VERSION)_amd64

XPCI_TMP := $(shell mktemp -u -d -t xpci.XXXXXXXXXX)

# Need to remove dependencies on linux packages because our Linux image is unsigned
# Also blaclisting xpci - it will be loaded later by platform start script
$(addprefix $(DEST)/, $(XPCI_DEB)): $(DEST)/% :
	rm -rf $(XPCI_DEB)

	wget $(XPCI_URL_PREFIX)/$(XPCI_DEB)

	# w/a: remove dependencies
	mkdir -p $(XPCI_TMP)/DEBIAN

	dpkg -e $(XPCI_DEB) $(XPCI_TMP)/DEBIAN
	dpkg -x $(XPCI_DEB) $(XPCI_TMP)

	sed -i '/^Depends:/c\Depends:' $(XPCI_TMP)/DEBIAN/control
	echo "echo 'blacklist xpci' >> /etc/modprobe.d/xpci.conf" >> $(XPCI_TMP)/DEBIAN/postinst

	mkdir -p $(REPACKED_XPCI_NAME)/DEBS
	pushd $(REPACKED_XPCI_NAME)/DEBS
	dpkg -b $(XPCI_TMP) .
	popd

	rm -rf $(XPCI_TMP)

	cp $(REPACKED_XPCI_NAME)/DEBS/*.deb $(DEST)

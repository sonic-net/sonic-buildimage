.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

XPCI_NAME =  xpci-dkms_$(XPCI_VERSION)_amd64
MOD_DEB_NAME = xpci-modules-$(KVERSION)
MOD_DEB = $(MOD_DEB_NAME)_$(XPCI_VERSION)_amd64.deb

MAIN_TARGET = $(MOD_DEB)

DKMS_BMDEB = /var/lib/dkms/xpci/$(XPCI_VERSION)/bmdeb
DKMS_TMP := $(shell mktemp -u -d -t dkms.XXXXXXXXXX)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :

	mkdir -p $(DKMS_TMP)/DEBIAN
	mkdir -p $(DKMS_TMP)/lib/modules/$(KVERSION)/updates/dkms

	export kversion="$(KVERSION)"
	export xpci_version="$(XPCI_VERSION)"
	export mod_deb_name="$(MOD_DEB_NAME)"
	j2 templates/control.j2 > $(DKMS_TMP)/DEBIAN/control
	j2 templates/postinst.j2 > $(DKMS_TMP)/DEBIAN/postinst
	chmod +x $(DKMS_TMP)/DEBIAN/postinst

	cp /lib/modules/$(KVERSION)/updates/dkms/xpci.ko $(DKMS_TMP)/lib/modules/$(KVERSION)/updates/dkms

	mkdir -p $(XPCI_NAME)/DEBS
	pushd $(XPCI_NAME)/DEBS
	dpkg -b $(DKMS_TMP) .
	popd

	rm -rf $(DKMS_TMP)

	cp $(XPCI_NAME)/DEBS/*.deb $(DEST)

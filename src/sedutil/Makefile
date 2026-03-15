# Makefile for sedutil package

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -ex

MAIN_TARGET = sedutil_$(SEDUTIL_VERSION)_$(CONFIGURED_ARCH).deb

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	wget $(SEDUTIL_GITHUB_URL)/releases/download/$(SEDUTIL_VERSION)/sedutil-cli-$(SEDUTIL_VERSION).zip
	unzip sedutil-cli-$(SEDUTIL_VERSION).zip

	# Create debian package structure
	mkdir -p debian-pkg/DEBIAN debian-pkg/usr/local/bin
	cp sedutil-cli debian-pkg/usr/local/bin/
	chmod 755 debian-pkg/usr/local/bin/sedutil-cli

	# Create control file
	echo "Package: sedutil" > debian-pkg/DEBIAN/control
	echo "Version: $(SEDUTIL_VERSION)" >> debian-pkg/DEBIAN/control
	echo "Architecture: $(CONFIGURED_ARCH)" >> debian-pkg/DEBIAN/control
	echo "Maintainer: SONiC Team" >> debian-pkg/DEBIAN/control
	echo "Description: SED management utility" >> debian-pkg/DEBIAN/control

	# Build the Debian package
	dpkg-deb --build debian-pkg $*
	mv $* $(DEST)/

	# Cleanup
	rm -rf debian-pkg sedutil-cli*

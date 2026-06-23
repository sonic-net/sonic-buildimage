.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(SYSTEMD_UKIFY)

UKIFY_URL = https://raw.githubusercontent.com/systemd/systemd/v256.2/src/ukify/ukify.py

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Download the official standalone systemd ukify tool from GitHub
	curl -o ukify -sSL $(UKIFY_URL)
	chmod +x ukify
	# Build the systemd-ukify debian package cleanly
	dpkg-buildpackage -rfakeroot -b -us -uc -d --admindir $(SONIC_DPKG_ADMINDIR)
	mv ../$(SYSTEMD_UKIFY) $(DEST)/

clean:
	rm -rf debian/files debian/*.substvars debian/systemd-ukify/ ukify

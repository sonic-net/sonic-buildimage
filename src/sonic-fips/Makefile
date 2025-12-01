.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

SONIC_FIPS_BUILD_FROM_SOURCE =? n

include ../../rules/sonic-fips.mk

MAIN_TARGET = $(SYMCRYPT_OPENSSL)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	if [ "$(SONIC_FIPS_BUILD_FROM_SOURCE)" == "y" ]; then
	  git clone -b "$(FIPS_VERSION)" https://github.com/Azure/sonic-fips
	  push sonic-fips
	  git submodule update --init
	  push src/SymCrypt; git submodule update --init -- jitterentropy-library; pop
	  make all
	  pop
	  cp sonic-fips/target/*.deb $(DEST)/
	  exit 0
	fi
	for target in $(FIPS_PACKAGE_ALL); do
	  filename=$$(basename $$target)
	  url=$(FIPS_URL_PREFIX)/$$filename
	  mkdir -p "$$(dirname $(DEST)/$$target)"
	  curl -f -o "$(DEST)/$$target" "$$url"
	  touch "$(DEST)/$$target"
	done

$(addprefix $(DEST)/, $(FIPS_DERIVED_TARGET)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

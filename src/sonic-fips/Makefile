.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

SONIC_FIPS_BUILD_FROM_SOURCE =? n

include ../../rules/sonic-fips.mk

MAIN_TARGET = $(SYMCRYPT_OPENSSL)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Explicitly build from source if requested
	if [ "$(SONIC_FIPS_BUILD_FROM_SOURCE)" == "y" ]; then \
	  git clone -b "$(FIPS_VERSION)" https://github.com/Azure/sonic-fips || true; \
	  if [ -d sonic-fips ]; then \
	    pushd sonic-fips; \
	    git submodule update --init || true; \
	    pushd src/SymCrypt; git submodule update --init -- jitterentropy-library || true; popd; \
	    make all || true; \
	    popd; \
	    if ls sonic-fips/target/*.deb >/dev/null 2>&1; then \
	      cp sonic-fips/target/*.deb $(DEST)/; \
	      exit 0; \
	    fi; \
	  fi; \
	fi; \
	# Try to download prebuilt packages first
	DOWNLOAD_FAILED=0; \
	for target in $(FIPS_PACKAGE_ALL); do \
	  filename=$$(basename $$target); \
	  url=$(FIPS_URL_PREFIX)/$$filename; \
	  mkdir -p "$$(dirname $(DEST)/$$target)"; \
	  if ! wget -O "$(DEST)/$$target" "$$url" 2>/dev/null; then \
	    echo "WARN: download failed for $$filename, will build from source"; \
	    DOWNLOAD_FAILED=1; \
	    rm -f "$(DEST)/$$target"; \
	  else \
	    touch "$(DEST)/$$target"; \
	  fi; \
	done; \
	if [ "$$DOWNLOAD_FAILED" = "0" ]; then \
	  exit 0; \
	fi; \
	# Fallback to building from source
	echo "INFO: building FIPS packages from source fallback"; \
	rm -rf sonic-fips; \
	git clone -b "$(FIPS_VERSION)" https://github.com/Azure/sonic-fips || exit 1; \
	pushd sonic-fips; \
	git submodule update --init || true; \
	pushd src/SymCrypt; git submodule update --init -- jitterentropy-library || true; popd; \
	make all; \
	popd; \
	cp sonic-fips/target/*.deb $(DEST)/

$(addprefix $(DEST)/, $(FIPS_DERIVED_TARGET)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

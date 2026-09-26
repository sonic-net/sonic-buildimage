.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

MAIN_TARGET = $(RSYSLOG)
DERIVED_TARGETS = rsyslog-dbgsym_$(RSYSLOG_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  rsyslog-relp_$(RSYSLOG_VERSION_FULL)_$(CONFIGURED_ARCH).deb \
		  rsyslog-relp-dbgsym_$(RSYSLOG_VERSION_FULL)_$(CONFIGURED_ARCH).deb

RSYSLOG_URL = https://deb.debian.org/debian/pool/main/r/rsyslog/

DSC_FILE = rsyslog_$(RSYSLOG_VERSION_FULL).dsc
DSC_FILE_URL = $(RSYSLOG_URL)/$(DSC_FILE)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	# Remove any stale files
	rm -rf ./rsyslog-$(RSYSLOG_VERSION)

	# Get rsyslog release
	dget -u "$(DSC_FILE_URL)"

	pushd rsyslog-$(RSYSLOG_VERSION)

	# Apply patches
	# Do NOT use stgit; otherwise rsyslog will detect that it's in a git
	# repository, compile with -Werror, and fail the build. Quilt is being
	# used by Debian for its own patches, so that leaves us with regular
	# patch.
	patch -p1 -i ../patch/6263-reordered-actionDestruc.patch
	patch -p1 -i ../patch/imuxsock-bound-per-pid-ratelimiter-cache.patch

	# Build Debian package
	# Disable running tests when building rsyslog. The TLS-related test cases (in particular, the ones
	# that appear to specify which TLS versions to use) are at least partly broken when running them
	# as part of the build. This may be because of the FIPS SSL libraries that are installed.
ifeq ($(CROSS_BUILD_ENVIRON), y)
	dpkg-buildpackage -b -us -uc -a$(CONFIGURED_ARCH) -Pcross,nocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
else
	dpkg-buildpackage -b -us -uc -Pnocheck -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $(SONIC_DPKG_ADMINDIR)
endif
	popd

	# Move the newly-built .deb packages to the destination directory
	mv $(DERIVED_TARGETS) $* $(DEST)/

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

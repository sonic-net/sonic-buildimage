# Integrate CodeConstruct mctp userspace tools:
# https://github.com/CodeConstruct/mctp
#
# Source-only build: the recipe fetches the upstream source tarball from
# MCTP_SOURCE_BASE_URL and builds the .deb locally (see mctp/Makefile).
# Override MCTP_SOURCE_BASE_URL to fetch the source tarball from a mirror.

# Git tag only for the GitHub archive tarball (refs/tags URL below; not a branch).
MCTP_UPSTREAM_TAG ?= v2.5

# Debian packaging revision (upstream-version is derived from the tag).
MCTP_PKG_RELEASE ?= 1

# Strip leading 'v' for upstream directory names (mctp-2.5) and changelog.
MCTP_DEB_VERSION = $(patsubst v%,%,$(MCTP_UPSTREAM_TAG))
MCTP_PKG_VERSION = $(MCTP_DEB_VERSION)-$(MCTP_PKG_RELEASE)

# GitHub archive base (default: build from source).
MCTP_SOURCE_BASE_URL ?= https://github.com/CodeConstruct/mctp/archive/refs/tags

MCTP_ARCHIVE_URL = $(MCTP_SOURCE_BASE_URL)/$(MCTP_UPSTREAM_TAG).tar.gz

# SHA-256 of the pinned $(MCTP_UPSTREAM_TAG) source tarball. The build verifies
# the download against this before extracting and fails closed if it is unset
# or mismatched. Update this whenever MCTP_UPSTREAM_TAG changes.
MCTP_ARCHIVE_SHA256 ?= 9cb001d64afbc03f656ef0852a9e616864096d2b0b1d7fcc15cfc4dbb3423bf9

MCTP = mctp_$(MCTP_PKG_VERSION)_$(CONFIGURED_ARCH).deb
$(MCTP)_SRC_PATH = $(PLATFORM_PATH)/mctp
# Lazy-installed on the NVIDIA AST2700 BMC platform (needs a _PLATFORM device
# so the lazy-install dev@deb mapping in slave.mk is well-formed).
$(MCTP)_PLATFORM = arm64-aspeed_nvidia_ast2700_bmc-r0

export MCTP_UPSTREAM_TAG MCTP_PKG_VERSION MCTP_DEB_VERSION MCTP_ARCHIVE_URL MCTP_ARCHIVE_SHA256 MCTP

SONIC_MAKE_DEBS += $(MCTP)

MCTP_PACKAGES = $(MCTP)
mctp-packages: $(addprefix $(DEBS_PATH)/, $(MCTP_PACKAGES))

SONIC_PHONY_TARGETS += mctp-packages

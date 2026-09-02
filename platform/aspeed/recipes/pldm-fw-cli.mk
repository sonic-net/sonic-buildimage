# Integrate CodeConstruct pldm-fw-cli (PLDM for Firmware Update UA) from mctp-rs:
# https://github.com/CodeConstruct/mctp-rs/tree/main/pldm-fw-cli

# Upstream Git tag (full mctp-rs tree required for workspace build).
PLDM_FW_UPSTREAM_TAG ?= pldm-fw-cli-0.2.0

# Upstream version derived from the tag (pldm-fw-cli-0.2.0 -> 0.2.0), which
# matches the Cargo.toml version in pldm-fw-cli at that tag.
PLDM_FW_UPSTREAM_VERSION = $(patsubst pldm-fw-cli-%,%,$(PLDM_FW_UPSTREAM_TAG))

PLDM_FW_PKG_RELEASE ?= 1
PLDM_FW_PKG_VERSION = $(PLDM_FW_UPSTREAM_VERSION)-$(PLDM_FW_PKG_RELEASE)

PLDM_FW_SOURCE_BASE_URL ?= https://github.com/CodeConstruct/mctp-rs/archive/refs/tags

PLDM_FW_ARCHIVE_URL = $(PLDM_FW_SOURCE_BASE_URL)/$(PLDM_FW_UPSTREAM_TAG).tar.gz

# SHA-256 of the pinned $(PLDM_FW_UPSTREAM_TAG) source tarball. The build
# verifies the download against this before extracting and fails closed if it
# is unset or mismatched. Update this whenever PLDM_FW_UPSTREAM_TAG changes.
PLDM_FW_ARCHIVE_SHA256 ?= 112ff61673c6c8f79bc1f24fa965a8144b910ba6f4d4802dc4fffb1f9def7fad

PLDM_FW = pldm-fw_$(PLDM_FW_PKG_VERSION)_$(CONFIGURED_ARCH).deb
$(PLDM_FW)_SRC_PATH = $(PLATFORM_PATH)/pldm-fw-cli

export PLDM_FW_UPSTREAM_TAG PLDM_FW_UPSTREAM_VERSION PLDM_FW_PKG_VERSION PLDM_FW_ARCHIVE_URL PLDM_FW_ARCHIVE_SHA256 PLDM_FW

SONIC_MAKE_DEBS += $(PLDM_FW)

PLDM_FW_PACKAGES = $(PLDM_FW)
pldm-fw-packages: $(addprefix $(DEBS_PATH)/, $(PLDM_FW_PACKAGES))

SONIC_PHONY_TARGETS += pldm-fw-packages

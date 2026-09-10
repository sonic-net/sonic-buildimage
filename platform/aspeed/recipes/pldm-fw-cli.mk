# Integrate CodeConstruct pldm-fw-cli (PLDM for Firmware Update UA) from mctp-rs:
# https://github.com/CodeConstruct/mctp-rs/tree/main/pldm-fw-cli

# Pin to a specific upstream mctp-rs commit (the full workspace tree is
# required for the cargo workspace build). Our local SONiC changes live as
# patches under pldm-fw-cli/patches/ and are generated against this exact
# commit, so it must stay a fixed SHA -- a moving ref (e.g. a branch) would
# let upstream drift and break patch application.
#
# b134e145 == CodeConstruct/mctp-rs origin/main at the time the patches were
# produced.
override PLDM_FW_UPSTREAM_COMMIT := b134e145f93d634dff7eb9f2a01559273c687365

# Matches the pldm-fw-cli Cargo.toml version at the pinned commit.
PLDM_FW_UPSTREAM_VERSION = 0.2.0

PLDM_FW_PKG_RELEASE ?= 1
PLDM_FW_PKG_VERSION = $(PLDM_FW_UPSTREAM_VERSION)-$(PLDM_FW_PKG_RELEASE)

# GitHub serves a tarball of an arbitrary commit at archive/<sha>.tar.gz.
PLDM_FW_SOURCE_BASE_URL ?= https://github.com/CodeConstruct/mctp-rs/archive

PLDM_FW_ARCHIVE_URL = $(PLDM_FW_SOURCE_BASE_URL)/$(PLDM_FW_UPSTREAM_COMMIT).tar.gz

# SHA256 of the upstream archive currently served for PLDM_FW_UPSTREAM_COMMIT.
# The build verifies the downloaded tarball against this before extraction and
# fails on mismatch, which detects tampering or unexpected content. Forced
# non-overridable like the commit pin. Note: GitHub's auto-generated
# archive/<sha>.tar.gz is not guaranteed byte-stable for a fixed commit --
# generator changes can alter the checksum without changing
# PLDM_FW_UPSTREAM_COMMIT. For true byte stability, use a controlled mirror or
# a pinned-SHA clone instead of the live GitHub archive URL. Regenerate this
# digest whenever the commit pin changes or GitHub's archive bytes drift.
override PLDM_FW_ARCHIVE_SHA256 := 3f0848eead41a56c579a9ee937ab97c9a7b765897795d3cb7d8d7f5f3845ea23

PLDM_FW = pldm-fw_$(PLDM_FW_PKG_VERSION)_$(CONFIGURED_ARCH).deb
$(PLDM_FW)_SRC_PATH = $(PLATFORM_PATH)/pldm-fw-cli
# Lazy-installed on the NVIDIA AST2700 BMC platform (needs a _PLATFORM device
# so the lazy-install dev@deb mapping in slave.mk is well-formed).
$(PLDM_FW)_PLATFORM = arm64-aspeed_nvidia_ast2700_bmc-r0

export PLDM_FW_UPSTREAM_COMMIT PLDM_FW_UPSTREAM_VERSION PLDM_FW_PKG_VERSION PLDM_FW_ARCHIVE_URL PLDM_FW_ARCHIVE_SHA256 PLDM_FW

SONIC_MAKE_DEBS += $(PLDM_FW)

PLDM_FW_PACKAGES = $(PLDM_FW)
pldm-fw-packages: $(addprefix $(DEBS_PATH)/, $(PLDM_FW_PACKAGES))

SONIC_PHONY_TARGETS += pldm-fw-packages

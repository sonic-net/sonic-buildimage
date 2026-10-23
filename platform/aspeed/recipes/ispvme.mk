#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Integrate NVIDIA ispvme (Lattice CPLD firmware update over JTAG):
# https://github.com/NVIDIA/ispvme
#
# Source-only build: the recipe fetches the upstream source tarball from
# ISPVME_SOURCE_BASE_URL and builds the .deb locally (see ispvme/Makefile).
# Override ISPVME_SOURCE_BASE_URL to fetch the source tarball from a mirror.

# Pin to a specific upstream ispvme commit. Forced non-overridable so a moving
# ref cannot silently change the tree that debian/ packages.
#
# 414cdf58 == NVIDIA/ispvme origin/main at the time this recipe was added.
override ISPVME_UPSTREAM_COMMIT := 414cdf580d3a96eed52029d4e9c0305d48f9e3d3

# Matches the meson project version at the pinned commit.
ISPVME_UPSTREAM_VERSION = 0.0.0

ISPVME_PKG_RELEASE ?= 1
ISPVME_PKG_VERSION = $(ISPVME_UPSTREAM_VERSION)-$(ISPVME_PKG_RELEASE)

# GitHub serves a tarball of an arbitrary commit at archive/<sha>.tar.gz.
ISPVME_SOURCE_BASE_URL ?= https://github.com/NVIDIA/ispvme/archive

ISPVME_ARCHIVE_URL = $(ISPVME_SOURCE_BASE_URL)/$(ISPVME_UPSTREAM_COMMIT).tar.gz

# SHA256 of the upstream archive currently served for ISPVME_UPSTREAM_COMMIT.
# The build verifies the downloaded tarball against this before extraction and
# fails on mismatch. Forced non-overridable like the commit pin. Note: GitHub's
# auto-generated archive/<sha>.tar.gz is not guaranteed byte-stable for a fixed
# commit -- generator changes can alter the checksum without changing
# ISPVME_UPSTREAM_COMMIT. Regenerate this digest whenever the commit pin
# changes or GitHub's archive bytes drift.
override ISPVME_ARCHIVE_SHA256 := f707000877c16e29387f472cbd265a8030819c654099e83cdcc7349a83c31e44

ISPVME = ispvme_$(ISPVME_PKG_VERSION)_$(CONFIGURED_ARCH).deb
$(ISPVME)_SRC_PATH = $(PLATFORM_PATH)/ispvme
# Lazy-installed on the NVIDIA AST2700 BMC platform (needs a _PLATFORM device
# so the lazy-install dev@deb mapping in slave.mk is well-formed).
$(ISPVME)_PLATFORM = arm64-aspeed_nvidia_ast2700_bmc-r0

export ISPVME_UPSTREAM_COMMIT ISPVME_UPSTREAM_VERSION ISPVME_PKG_VERSION ISPVME_ARCHIVE_URL ISPVME_ARCHIVE_SHA256 ISPVME

SONIC_MAKE_DEBS += $(ISPVME)

ISPVME_PACKAGES = $(ISPVME)
ispvme-packages: $(addprefix $(DEBS_PATH)/, $(ISPVME_PACKAGES))

SONIC_PHONY_TARGETS += ispvme-packages

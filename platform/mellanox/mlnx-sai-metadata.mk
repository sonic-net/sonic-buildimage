#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Mellanox-only hook: saitypescustom.h from mlnx-sai is copied into
# sonic-sairedis/SAI/custom/ before parse.pl generates saimetadata.c so
# SAI_ACL_BIND_POINT_TYPE_CPU_PORT is merged into libsaimetadata.so.

MLNX_SAI_CUSTOM_HEADERS_SCRIPT = $(BUILD_WORKDIR)/$(PLATFORM_PATH)/scripts/sync_mlnx_sai_custom_headers.sh
MLNX_SAI_CUSTOM_DIR = $(BUILD_WORKDIR)/$($(LIBSAIREDIS)_SRC_PATH)/SAI/custom

# Ensure the mlnx-sai .deb is built before libsairedis so the sync hook can
# extract saitypescustom.h from it (via dpkg-deb -x). Use _AFTER, not _DEPENDS:
# the header is only read out of the .deb file, so mlnx-sai must NOT be
# installed. Installing it here would leave mlnx-sai present (libsairedis has no
# UNINSTALLS) and deadlock the later libsaivs-dev-install, which conflicts with
# mlnx-sai. Per platform/mellanox/rules.mk, mlnx-sai is installed only for syncd
# (which uninstalls it again).
$(LIBSAIREDIS)_AFTER += $(MLNX_SAI)

# libsairedis (rules/sairedis.mk) and syncd (rules/syncd.mk) each run their own
# dpkg-buildpackage of the sonic-sairedis source, and both recompile SAI/meta, so
# both must resolve the "#include <saicustom.h>" that parse.pl emits into the
# generated saimetadata.h once SAI/custom is non-empty. The shared pre-build env
# below syncs the vendor custom headers into SAI/custom/, then puts SAI/custom on
# the include path for the two build steps that need it:
#   - C/C++ compiles: dpkg-buildflags' CPPFLAGS is on every compile line, so
#     DEB_CPPFLAGS_MAINT_APPEND appends -I<custom>.
#   - the pysairedis SWIG step: swig takes its -I from SAIINC (configure.ac), not
#     CPPFLAGS, but it also honours the SWIG_FEATURES env var, so we inject
#     -I<custom> there too.
# This keeps the include-path fix entirely in sonic-buildimage (no sonic-sairedis
# source change). Absolute paths: the build runs after pushd into sonic-sairedis.
MLNX_SAI_CUSTOM_BUILD_ENV = $(MLNX_SAI_CUSTOM_HEADERS_SCRIPT) $(BUILD_WORKDIR)/$(DEBS_PATH)/$(MLNX_SAI) $(MLNX_SAI_CUSTOM_DIR) && DEB_CPPFLAGS_MAINT_APPEND="-I$(MLNX_SAI_CUSTOM_DIR)" SWIG_FEATURES="-I$(MLNX_SAI_CUSTOM_DIR)"

$(LIBSAIREDIS)_BUILD_ENV = $(MLNX_SAI_CUSTOM_BUILD_ENV)

# syncd is a second, independent dpkg-buildpackage of the same source, so it
# needs the identical handling. mlnx-sai is already a _DEPENDS of syncd (per
# platform/mellanox/rules.mk it is installed for the syncd build, then
# uninstalled), so its .deb is present for the sync hook to read.
$(SYNCD)_BUILD_ENV = $(MLNX_SAI_CUSTOM_BUILD_ENV)

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

BAZEL_TARGET_PATH = p4rt_app
BAZEL_BUILD_TARGETS = //p4rt_app:p4rt_deb //p4rt_app:p4rt_dbg_deb

# Enable debug symbols for remote debugging (generate a .dwp).
BAZEL_BUILD_OPTS += --copt=-gsplit-dwarf

# Override sonic_swss_common to use the version specific to this buildimage.
BAZEL_BUILD_OPTS += --override_repository=sonic_swss_common=/sonic/src/sonic-swss-common

# sonic-swss-common cannot generate cfg_schema.h under Bazel yet,
# so we hand it the real YANG-generated one that libswsscommon-dev installs (staged into CFG_SCHEMA_REPO below).
# This keeps YANG modules enabled, so p4rt links the same libswsscommon it always has.
CFG_SCHEMA_H = /usr/include/swss/cfg_schema.h
CFG_SCHEMA_REPO = /tmp/sonic-p4rt-cfg-schema
BAZEL_BUILD_OPTS += --override_repository=swss_cfg_schema=$(CFG_SCHEMA_REPO)
BAZEL_BUILD_OPTS += --@sonic_swss_common//tools/bazel:cfg_schema=@swss_cfg_schema//:cfg_schema.h

# Newer version of gcc has spurious warnings in boringssl package.
BAZEL_BUILD_OPTS += --copt=-Wno-array-bounds
BAZEL_BUILD_OPTS += --copt=-Wno-stringop-overflow

# Build optimized, stripped binaries.
BAZEL_BUILD_OPTS += -c opt

# Use a host directory to store Bazel cache, to speed up incremental rebuilds
# on the same system for developers that make changes to sonic-pins code.
#
# /bazel_cache is the shared Bazel cache mount every slave container gets
# (see SONIC_BAZEL_CACHE_SOURCE in rules/config), so this works out of the box.
# To point it elsewhere instead, override BAZEL_CACHE and mount it yourself via
# DOCKER_BUILDER_USER_MOUNT:
#   DOCKER_BUILDER_USER_MOUNT=<directory on host system>:$BAZEL_CACHE:rw
#
BAZEL_CACHE ?= /bazel_cache
BAZEL_OPTS += $(shell test -d $(BAZEL_CACHE) && echo --output_user_root=$(BAZEL_CACHE)/output_user_root)

MAIN_TARGET = $(SONIC_P4RT)
DERIVED_TARGETS = $(SONIC_P4RT_DBG)

# TODO: Remove this Makefile workaround once Bazel supports generating cfg_schema.h.
#
# Wrap the installed header in a throwaway repository, so that it can be named by a label
# and handed to --@sonic_swss_common//tools/bazel:cfg_schema.
$(CFG_SCHEMA_REPO)/BUILD: $(CFG_SCHEMA_H)
	mkdir -p $(@D)
	cp $< $(@D)/cfg_schema.h
	echo 'workspace(name = "swss_cfg_schema")' > $(@D)/WORKSPACE
	echo 'exports_files(["cfg_schema.h"])' > $@

$(CFG_SCHEMA_H):
	echo "$@ not found: is libswsscommon-dev installed?" >&2
	exit 1

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% : $(CFG_SCHEMA_REPO)/BUILD
	function cleanup {
		# Note: make seems to hang if Bazel is still running
		cd $(CURDIR)/sonic-pins && bazel $(BAZEL_OPTS) shutdown
	}
	trap cleanup EXIT

	pushd ./sonic-pins
	bazel $(BAZEL_OPTS) build $(BAZEL_BUILD_OPTS) $(BAZEL_BUILD_TARGETS)
	bazel $(BAZEL_OPTS) test $(BAZEL_BUILD_OPTS) //$(BAZEL_TARGET_PATH)/...
	cd ./bazel-bin/$(BAZEL_TARGET_PATH)
	mv $* $(DERIVED_TARGETS) $(DEST)/
	popd

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

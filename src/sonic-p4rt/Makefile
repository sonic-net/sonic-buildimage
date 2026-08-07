.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

BAZEL_TARGET_PATH = p4rt_app
BAZEL_BUILD_TARGETS = //p4rt_app:p4rt_deb //p4rt_app:p4rt_dbg_deb

# Enable debug symbols for remote debugging (generate a .dwp).
BAZEL_BUILD_OPTS += --copt=-gsplit-dwarf

# Override sonic_swss_common to use a pinned version of sonic-swss-common.
# Note that this is **not up to date** with src/sonic-swss-common, because that version needs bzlmod to function,
# and sonic-p4rt is built using Bazel 6.4.0.
# TODO(bazel-ready): Remove when we migrate p4rt to Bazel 8.5.1.
BAZEL_BUILD_OPTS += --override_repository=sonic_swss_common=/sonic/src/sonic-p4rt/sonic-swss-common-bazel-6

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

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	function cleanup {
		# Note: make seems to hang if Bazel is still running
		cd $(CURDIR)/sonic-pins && bazel $(BAZEL_OPTS) shutdown
	}
	trap cleanup EXIT

	# TODO: Remove this Makefile workaround
	# once Bazel natively supports generating cfg_schema.h.
	if [[ ! -f /sonic/src/sonic-p4rt/sonic-swss-common-bazel-6/common/cfg_schema.h && -f /usr/include/swss/cfg_schema.h ]]; then \
		if [ ! -d /sonic/src/sonic-p4rt/sonic-swss-common-bazel-6/common ]; then \
			mkdir -p /sonic/src/sonic-p4rt/sonic-swss-common-bazel-6/common; \
		fi; \
		cp /usr/include/swss/cfg_schema.h /sonic/src/sonic-p4rt/sonic-swss-common-bazel-6/common/cfg_schema.h; \
	fi

	pushd ./sonic-pins
	bazel $(BAZEL_OPTS) build $(BAZEL_BUILD_OPTS) $(BAZEL_BUILD_TARGETS)
	bazel $(BAZEL_OPTS) test $(BAZEL_BUILD_OPTS) //$(BAZEL_TARGET_PATH)/...
	cd ./bazel-bin/$(BAZEL_TARGET_PATH)
	mv $* $(DERIVED_TARGETS) $(DEST)/
	popd

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

BAZEL_TARGET_PATH = p4rt_app
BAZEL_BUILD_TARGETS = //p4rt_app:p4rt_deb //p4rt_app:p4rt_dbg_deb

# Enable debug symbols for remote debugging (generate a .dwp).
BAZEL_BUILD_OPTS += --copt=-gsplit-dwarf

# Override sonic_swss_common to use the version specific to this buildimage.
BAZEL_BUILD_OPTS += --override_repository=sonic_swss_common=/sonic/src/sonic-swss-common

# Newer version of gcc has spurious warnings in boringssl package.
BAZEL_BUILD_OPTS += --copt=-Wno-array-bounds
BAZEL_BUILD_OPTS += --copt=-Wno-stringop-overflow

# Build optimized, stripped binaries.
BAZEL_BUILD_OPTS += -c opt

# Use a host directory to store Bazel cache, if mounted. This will speed up
# incremental rebuilds on the same system for developers that makes changes
# to sonic-pins code.
#
# To build with Bazel cache, add DOCKER_BUILDER_USER_MOUNT:
#   DOCKER_BUILDER_USER_MOUNT=<directory on host system>:$BAZEL_CACHE:rw
#
# For example:
#   DOCKER_BUILDER_USER_MOUNT=/tmp/bazel_cache:/bazel:rw make target/...
#
BAZEL_CACHE ?= /bazel
BAZEL_OPTS += $(shell test -d $(BAZEL_CACHE) && echo --output_user_root=$(BAZEL_CACHE)/cache)

MAIN_TARGET = $(SONIC_P4RT)
DERIVED_TARGETS = $(SONIC_P4RT_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
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

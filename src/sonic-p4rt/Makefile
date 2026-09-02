.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

BAZEL_TARGET_PATH = p4rt_app
BAZEL_BUILD_TARGETS = //p4rt_app:p4rt_deb //p4rt_app:p4rt_dbg_deb

# Bazel's downloader bypasses the SONiC wget hook. Prefetch the archives that
# failed in the Official build, then let Bazel consume them from a distdir.
BAZEL_DISTDIR = $(CURDIR)/bazel-distdir
GRPC_ARCHIVE = v1.63.0.zip
GRPC_ARCHIVE_URL = https://github.com/grpc/grpc/archive/$(GRPC_ARCHIVE)
GRPC_ARCHIVE_SHA256 = daa1b06a19b5f7e4603e1f8980eeab43cf69b6e89bee3b2547f275fa5af7f480
GOOGLEAPIS_ARCHIVE = f405c718d60484124808adb7fb5963974d654bb4.zip
GOOGLEAPIS_ARCHIVE_URL = https://github.com/googleapis/googleapis/archive/$(GOOGLEAPIS_ARCHIVE)
GOOGLEAPIS_ARCHIVE_SHA256 = 406b64643eede84ce3e0821a1d01f66eaf6254e79cb9c4f53be9054551935e79

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
BAZEL_BUILD_OPTS += --distdir=$(BAZEL_DISTDIR)

MAIN_TARGET = $(SONIC_P4RT)
DERIVED_TARGETS = $(SONIC_P4RT_DBG)

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	function cleanup {
		# Note: make seems to hang if Bazel is still running
		cd $(CURDIR)/sonic-pins && bazel $(BAZEL_OPTS) shutdown
		rm -rf $(BAZEL_DISTDIR)
	}
	trap cleanup EXIT

	grep -Fq 'url = "$(GRPC_ARCHIVE_URL)"' sonic-pins/pins_infra_deps.bzl
	grep -Fq 'sha256 = "$(GRPC_ARCHIVE_SHA256)"' sonic-pins/pins_infra_deps.bzl
	grep -Fq 'url = "$(GOOGLEAPIS_ARCHIVE_URL)"' sonic-pins/pins_infra_deps.bzl
	grep -Fq 'sha256 = "$(GOOGLEAPIS_ARCHIVE_SHA256)"' sonic-pins/pins_infra_deps.bzl

	rm -rf $(BAZEL_DISTDIR)
	mkdir -p $(BAZEL_DISTDIR)
	wget --no-verbose --tries=3 -O $(BAZEL_DISTDIR)/$(GRPC_ARCHIVE) $(GRPC_ARCHIVE_URL)
	echo "$(GRPC_ARCHIVE_SHA256)  $(BAZEL_DISTDIR)/$(GRPC_ARCHIVE)" | sha256sum -c -
	wget --no-verbose --tries=3 -O $(BAZEL_DISTDIR)/$(GOOGLEAPIS_ARCHIVE) $(GOOGLEAPIS_ARCHIVE_URL)
	echo "$(GOOGLEAPIS_ARCHIVE_SHA256)  $(BAZEL_DISTDIR)/$(GOOGLEAPIS_ARCHIVE)" | sha256sum -c -

	# TODO: Remove this Makefile workaround
	# once Bazel natively supports generating cfg_schema.h.
	if [[ ! -f /sonic/src/sonic-swss-common/common/cfg_schema.h && -f /usr/include/swss/cfg_schema.h ]]; then \
		if [ ! -d /sonic/src/sonic-swss-common/common ]; then \
			mkdir -p /sonic/src/sonic-swss-common/common; \
		fi; \
		cp /usr/include/swss/cfg_schema.h /sonic/src/sonic-swss-common/common/cfg_schema.h; \
	fi

	pushd ./sonic-pins
	bazel $(BAZEL_OPTS) build $(BAZEL_BUILD_OPTS) $(BAZEL_BUILD_TARGETS)
	bazel $(BAZEL_OPTS) test $(BAZEL_BUILD_OPTS) //$(BAZEL_TARGET_PATH)/...
	cd ./bazel-bin/$(BAZEL_TARGET_PATH)
	mv $* $(DERIVED_TARGETS) $(DEST)/
	popd

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

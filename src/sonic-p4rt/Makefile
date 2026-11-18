.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

BAZEL_TARGET_PATH = p4rt_app
BAZEL_BUILD_TARGETS = //p4rt_app:p4rt_deb //p4rt_app:p4rt_dbg_deb

# Keep the pinned archives and integrity checks while providing reachable
# public mirrors for this nested Bazel workspace's source downloads.
BAZEL_BUILD_OPTS += --experimental_downloader_config=$(CURDIR)/source_mirrors.cfg

# Let Bazel collect split debug symbols into the .dwp for optimized builds.
# Explicit -g overrides the toolchain's optimized-build default of -g0.
# GNU dwp 2.44 cannot package GCC 14's default DWARF 5 debug information.
BAZEL_BUILD_OPTS += --fission=opt --copt=-g --copt=-gdwarf-4

# Common builds with Bazel 8.5.1; P4RT imports its library and headers with 6.4.0.
COMMON_SOURCE ?= $(abspath $(CURDIR)/../sonic-swss-common)
COMMON_SDK := $(CURDIR)/.common-sdk
COMMON_SDK_BUILDER ?= $(abspath $(CURDIR)/../../tools/bazel/p4rt_common_sdk.py)
BAZEL_BUILD_OPTS += --override_repository=sonic_swss_common=$(COMMON_SDK)

# Newer version of gcc has spurious warnings in boringssl package.
BAZEL_BUILD_OPTS += --copt=-Wno-array-bounds
BAZEL_BUILD_OPTS += --copt=-Wno-stringop-overflow

# Keep optimized binaries with the skeleton debug information for the .dwp.
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
ifneq ($(wildcard $(BAZEL_CACHE)/.),)
COMMON_CACHE_OPTS = --bazel-arg=--repository_cache=$(abspath $(BAZEL_CACHE))/repository_cache --bazel-arg=--disk_cache=$(abspath $(BAZEL_CACHE))/disk_cache
endif

MAIN_TARGET = $(SONIC_P4RT)
DERIVED_TARGETS = $(SONIC_P4RT_DBG)

ifeq ($(SONIC_VS_IMAGE_TIMINGS),1)
vs_timer = python3 -B "$(CURDIR)/../../tools/bazel/ci/time_vs_image.py"
define vs_start_timing
	timing_directory="$$($(vs_timer) start-recipe "$(abspath $(CURDIR)/../..)" sonic-p4rt --bazel build test info shutdown --profiles build test --exports stage-debs)"
	exec > >(tee "$$timing_directory/make.log") 2>&1
endef
vs_time_phase = $(vs_timer) "$$timing_directory" $(1)
endif

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	$(vs_start_timing)
	# Recheck Common's source graph on every P4RT build, including cfg_schema.h.
	python3 "$(COMMON_SDK_BUILDER)" --common-source "$(COMMON_SOURCE)" \
		--output-dir "$(COMMON_SDK)" $(COMMON_CACHE_OPTS)

	cd ./sonic-pins
	source "$(CURDIR)/../../tools/bazel/private_server.sh"
	startup_options=($(BAZEL_OPTS))
	build_options=($(BAZEL_BUILD_OPTS) --symlink_prefix=/)
	if [[ -d "$(BAZEL_CACHE)" ]]; then
		startup_options+=("--output_user_root=$(abspath $(BAZEL_CACHE))/cache")
		build_options+=("--disk_cache=$(abspath $(BAZEL_CACHE))/disk_cache")
	fi
	if [[ -n "$${SONIC_BAZEL_REMOTE_CACHE:-}" ]]; then
		build_options+=("--remote_cache=$$SONIC_BAZEL_REMOTE_CACHE" "--remote_upload_local_results=$(if $(filter y,$(SONIC_BAZEL_REMOTE_UPLOAD)),true,false)")
	fi
	# The nested sonic-pins workspace selects its pinned Bazel version via PATH.
	export USE_BAZEL_VERSION=6.4.0
	BAZEL=bazel start_private_bazel sonic-p4rt "$${startup_options[@]}"
	private_bazel_shutdown_prefix=($(call vs_time_phase,shutdown))
	$(call vs_time_phase,build) "$${private_bazel[@]}" build "$${build_options[@]}" $(BAZEL_BUILD_TARGETS)
	$(call vs_time_phase,test) "$${private_bazel[@]}" test "$${build_options[@]}" //$(BAZEL_TARGET_PATH)/...
	package_bin="$$($(call vs_time_phase,info) "$${private_bazel[@]}" info "$${build_options[@]}" bazel-bin)"
	$(call vs_time_phase,stage-debs) mv -- $(foreach package,$* $(DERIVED_TARGETS),"$$package_bin/$(BAZEL_TARGET_PATH)/$(package)") "$(abspath $(DEST))/"

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

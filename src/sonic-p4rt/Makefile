.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e

BAZEL_TARGET_PATH = p4rt_app
BAZEL_BUILD_TARGETS = //p4rt_app:p4rt_deb //p4rt_app:p4rt_dbg_deb

# Keep the pinned archives and integrity checks while providing reachable
# public mirrors for this nested Bazel workspace's source downloads.
BAZEL_BUILD_OPTS += --experimental_downloader_config=$(CURDIR)/source_mirrors.cfg

ifeq ($(CONFIGURED_ARCH),arm64)
BAZEL_BUILD_OPTS += --config=aarch64
endif

# Newer version of gcc has spurious warnings in boringssl package.
BAZEL_BUILD_OPTS += --copt=-Wno-array-bounds
BAZEL_BUILD_OPTS += --copt=-Wno-stringop-overflow

BAZEL_BUILD_OPTS += --jobs=auto
BAZEL_BUILD_OPTS += --local_ram_resources=HOST_RAM*0.5
BAZEL_BUILD_OPTS += --local_cpu_resources=HOST_CPUS*0.5
BAZEL_BUILD_OPTS += --local_test_jobs=1

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

MAIN_TARGET = $(SONIC_P4RT)
DERIVED_TARGETS = $(SONIC_P4RT_DBG)

ifeq ($(SONIC_VS_IMAGE_TIMINGS),1)
vs_timer = python3 -B "$(CURDIR)/../../tools/bazel/ci/time_vs_image.py"
define vs_start_timing
	timing_directory="$$($(vs_timer) start-recipe "$(abspath $(CURDIR)/../..)" sonic-p4rt --bazel build test cquery shutdown --profiles build test --exports stage-debs)"
	exec > >(tee "$$timing_directory/make.log") 2>&1
endef
vs_time_phase = $(vs_timer) "$$timing_directory" $(1)
endif

$(addprefix $(DEST)/, $(MAIN_TARGET)): $(DEST)/% :
	$(vs_start_timing)
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
	export USE_BAZEL_VERSION=8.5.1
	BAZEL=bazel start_private_bazel sonic-p4rt "$${startup_options[@]}"
	private_bazel_shutdown_prefix=($(call vs_time_phase,shutdown))
	$(call vs_time_phase,build) "$${private_bazel[@]}" build "$${build_options[@]}" $(BAZEL_BUILD_TARGETS)
	$(call vs_time_phase,test) "$${private_bazel[@]}" test "$${build_options[@]}" //$(BAZEL_TARGET_PATH)/...
	execution_root="$$(private_bazel_execution_root)"
	package_output="$$($(call vs_time_phase,cquery) "$${private_bazel[@]}" cquery "$${build_options[@]}" --output=files $(firstword $(BAZEL_BUILD_TARGETS)))"
	package_bin="$$execution_root/$$(dirname -- "$$(dirname -- "$$package_output")")"
	$(call vs_time_phase,stage-debs) mv -- $(foreach package,$* $(DERIVED_TARGETS),"$$package_bin/$(BAZEL_TARGET_PATH)/$(package)") "$(abspath $(DEST))/"

$(addprefix $(DEST)/, $(DERIVED_TARGETS)): $(DEST)/% : $(DEST)/$(MAIN_TARGET)

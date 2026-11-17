# Use Make's existing metadata evaluation, service detection and JSON merge.
# Each owner registers manifest keys and the archive whose metadata they use:
#   SONIC_BAZEL_MANIFESTS += example example-dbg
#   example_MANIFEST_IMAGE = docker-example.gz
#   example-dbg_MANIFEST_IMAGE = docker-example.gz
#   example-dbg_MANIFEST_SUFFIX = dbg
# Add the resulting files to the container's _BAZEL_DEPENDS. A debug image that
# extends a runtime image needs both manifests, even when requested on its own.
.PHONY: bazel-manifests-force
bazel-manifests-force:

# Check on each request: metadata may come from conditionals, command-line
# settings or optional templates. Compare bytes before replacing a published
# input, so unchanged manifests do not invalidate downstream Make targets.
$(addprefix $(TARGET_PATH)/bazel-manifests/,$(addsuffix /manifest.json,$(sort $(SONIC_BAZEL_MANIFESTS)))) : $(TARGET_PATH)/bazel-manifests/%/manifest.json : bazel-manifests-force
	$(if $(value generate_manifest),,$(error include rules/functions before tools/bazel/manifests.mk))
	$(if $(strip $($*_MANIFEST_IMAGE)),,$(error $*_MANIFEST_IMAGE is required))
	$(if $(strip $($($*_MANIFEST_IMAGE)_PATH)),,$(error $($*_MANIFEST_IMAGE)_PATH is required))
	@mkdir -p "$(@D)"
	@manifest_stage=$$(mktemp -d "$(@D)/.manifest.XXXXXX")
	trap 'rm -rf "$$manifest_stage"' EXIT
	$(call generate_manifest,$(patsubst %.gz,%,$($*_MANIFEST_IMAGE)),$($*_MANIFEST_SUFFIX),$$manifest_stage)
	jq -e 'type == "object"' "$$manifest_stage/manifest.json" > /dev/null
	if ! cmp -s "$$manifest_stage/manifest.json" "$@"; then
		mv -f "$$manifest_stage/manifest.json" "$@"
	fi

# Owners opt in with SONIC_BAZEL_IMAGE_ENVIRONMENTS += docker-example and add
# target/bazel-manifests/docker-example/image.env to their _BAZEL_DEPENDS.
# This is Make's SONIC_IMAGE_VERSION, not the package version in manifest.json.
# Pass it through the environment so shell quoting cannot alter its contents.
SONIC_BAZEL_IMAGE_ENVIRONMENT_FILES := $(addprefix $(TARGET_PATH)/bazel-manifests/,$(addsuffix /image.env,$(sort $(SONIC_BAZEL_IMAGE_ENVIRONMENTS))))
ifneq ($(strip $(SONIC_BAZEL_IMAGE_ENVIRONMENT_FILES)),)
$(SONIC_BAZEL_IMAGE_ENVIRONMENT_FILES): private export SONIC_IMAGE_VERSION := $(SONIC_IMAGE_VERSION)
$(SONIC_BAZEL_IMAGE_ENVIRONMENT_FILES): $(TARGET_PATH)/bazel-manifests/%/image.env : tools/bazel/prepare_image_environment.py bazel-manifests-force
	python3 -B tools/bazel/prepare_image_environment.py --output "$@"
endif

# Component labels are a declared input for images that other SONiC packages
# depend on. Use the same label selection as the native Docker invocation.
SONIC_BAZEL_COMPONENT_LABEL_FILES := $(addprefix $(TARGET_PATH)/bazel-manifests/,$(addsuffix /components.labels,$(sort $(SONIC_BAZEL_COMPONENT_LABELS))))
ifneq ($(strip $(SONIC_BAZEL_COMPONENT_LABEL_FILES)),)
$(SONIC_BAZEL_COMPONENT_LABEL_FILES): private export COMPONENT_LABELS = $(call docker_component_version_labels,$($*_MANIFEST_IMAGE))
$(SONIC_BAZEL_COMPONENT_LABEL_FILES): $(TARGET_PATH)/bazel-manifests/%/components.labels : bazel-manifests-force
	@mkdir -p "$(@D)"
	stage=$$(mktemp "$(@D)/.labels.XXXXXX")
	trap 'rm -f "$$stage"' EXIT
	set -f
	for label in $$COMPONENT_LABELS; do printf '%s\n' "$$label"; done > "$$stage"
	if ! cmp -s "$$stage" "$@"; then mv -f "$$stage" "$@"; fi
endif

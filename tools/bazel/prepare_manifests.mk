# Prepare manifests for direct Bazel validation, without building containers.
# The production path includes manifests.mk from slave.mk's full Make context.
# Here the caller supplies the owning Make metadata and its build settings.
SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c
.ONESHELL:
.DEFAULT_GOAL := manifests
TARGET_PATH ?= target

ifeq ($(strip $(MANIFEST_METADATA)),)
$(error MANIFEST_METADATA must name the owning Make include)
endif
include rules/functions
include $(MANIFEST_METADATA)
include tools/bazel/manifests.mk

.PHONY: manifests
manifests: $(addprefix $(TARGET_PATH)/bazel-manifests/,$(addsuffix /manifest.json,$(sort $(SONIC_BAZEL_MANIFESTS))))
	$(if $(strip $(SONIC_BAZEL_MANIFESTS)),,$(error No manifests registered by $(MANIFEST_METADATA)))

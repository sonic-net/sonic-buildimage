#!/usr/bin/env bash
# Historical source-image drivers invoke this entry point with BAZEL_REAL and
# SONIC_BAZEL_REGISTRY_REVISION. Ordinary and base reviews use libteam directly.
exec "$(dirname "$0")/libteam_draft_bazel.sh" --source-image-review "$@"

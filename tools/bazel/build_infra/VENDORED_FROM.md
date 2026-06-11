# Vendored from sonic-build-infra

Source: https://github.com/thesayyn/sonic-build-infra
Commit: 4ea04ee (feat: Use local registry from sonic-buildimage)
Vendored: 2026-06-11

This directory is the in-tree copy of the (still unofficial) sonic-build-infra
repo, vendored here because:

1. There is no `sonic-net/sonic-build-infra` upstream yet.
2. The Bazel docker-orchagent build needs `@sonic_build_infra//python:site_packages.bzl`,
   `//tar:assert_tar.bzl`, `//swig:gen.bzl`, the `platform_*` config_settings,
   and the `bind.bzl` aliases.
3. Adding a new submodule pointing at a personal fork would violate the
   "main repo + submodules stay on official sonic-net URLs" rule for this
   port.

Bazel sees it as `@sonic_build_infra//...` because the root MODULE.bazel
binds it via `local_path_override(module_name = "sonic-build-infra",
path = "tools/bazel/build_infra")`.

If a real `sonic-net/sonic-build-infra` repo is ever created upstream, this
directory should be removed and `.gitmodules` updated to track it as a real
submodule.

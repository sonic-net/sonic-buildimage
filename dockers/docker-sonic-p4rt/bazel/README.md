# `docker-sonic-p4rt` Bazel 8 Native Assembly

`//dockers/docker-sonic-p4rt:docker-sonic-p4rt` and
`//dockers/docker-sonic-p4rt:docker-sonic-p4rt-dbg` build directly inside the
shared Bazel 8.5.1 Bzlmod workspace using `@sonic_pins//p4rt_app:p4rt_runtime`
without a nested Bazel 6.4.0 workspace or Common SDK handoff.

## Direct Targets

```sh
USE_BAZEL_VERSION=8.5.1 bazel build \
  --platforms=@sonic_build_infra//platforms:x86_64_trixie \
  --host_platform=@sonic_build_infra//platforms:x86_64_trixie \
  //dockers/docker-sonic-p4rt:docker-sonic-p4rt.gz \
  //dockers/docker-sonic-p4rt:docker-sonic-p4rt-dbg.gz
```

`docker-sonic-p4rt` assembles the runtime layer from:

- Base layers from `//dockers/docker-config-engine-trixie`.
- Native `@sonic_pins//p4rt_app:p4rt_runtime` (`p4rt` and helper binaries).
- Trixie runtime Debian dependencies (`runtime_apt_selection`, `runtime_debian`).
- Local files and templates (`files`: `supervisord.conf`, `p4rt.sh`,
  `p4rt_vars.j2`, and manifest labels).

`docker-sonic-p4rt-dbg` adds:

- `.build-id` split debug symbols from `:docker-sonic-p4rt.debug_symbols`
  (`debug_symbols_layer`), alongside `debug_tools`, `debug_client`,
  `debug_sai_runtime`, and their `.build-id` symbol layers (`:all_debug_symbols`).

## CI Validation Phases

In `p4rt_native`:

1. `tools/bazel/ci/p4rt_image.py build` audits the Bazel action graph, builds
   the runtime and debug OCI images and supporting layers/metadata, and records
   `resolution.collect` (`module-graph.json`, `source-pins.json`,
   `resolved-modules.json`).
2. `tools/bazel/ci/p4rt_validate.py validate` validates both OCI layouts,
   APT selections, package metadata/status files, `source_runtime` ELF linkage,
   and `.build-id` debug symbol coverage via `base_images.verify_symbols`.
3. `tools/bazel/ci/p4rt_validate.py incremental` modifies and restores
   `dockers/docker-sonic-p4rt/p4rt_vars.j2` to confirm incremental rebuilds
   keep base and unchanged runtime layers intact.
4. `tools/bazel/ci/p4rt_image.py owner-tests` runs `@sonic_pins//p4rt_app:runtime_package_test`
   natively in the shared Bazel 8.5.1 build.

In `p4rt_runtime`, `tools/bazel/ci/p4rt_loaded.py` loads `docker-sonic-p4rt.gz`
and `docker-sonic-p4rt-dbg.gz` in Docker and executes
`dockers/docker-sonic-p4rt/bazel/runtime_probe.py` (via `runtime_smoke`), then
`tools/bazel/ci/p4rt_acceptance.py` verifies `p4rt_acceptance.json` contracts
including `selected_modules` from `bazel mod graph`.

## Make Equivalence

After preparing Make manifests, compare Make `BUILDING_DOCKER` outputs against
`//dockers/docker-sonic-p4rt:docker-sonic-p4rt` (`docker-sonic-p4rt.gz` and
`docker-sonic-p4rt-dbg.gz`) via `tools/bazel/test_equivalence.sh` (using `//`
target paths rather than `--workspace` inside `sonic-buildimage`).

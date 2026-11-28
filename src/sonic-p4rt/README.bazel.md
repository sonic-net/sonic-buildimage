# P4RT builds natively with Bazel 8.5.1 and Bzlmod

`src/sonic-p4rt/sonic-pins` builds directly with **Bazel 8.5.1** using Bzlmod
(`MODULE.bazel`) against `sonic-swss-common`, `p4c`, and `sonic-build-infra` from
`sonic-bazel-registry` and the Bazel Central Registry. There is no separate
Bazel 6.4.0 bridge or Common SDK generation step.

When `BUILD_WITH_BAZEL_WHEN_AVAILABLE=y` selects the registered VS Trixie OCI
image build, `//dockers/docker-sonic-p4rt:docker-sonic-p4rt` consumes
`@sonic_pins//p4rt_app:p4rt_runtime` directly in the shared Bazel 8.5.1 build,
and `docker-sonic-p4rt-dbg` collects `.build-id` debug symbols via
`debug_symbols_layer`.

## Legacy Debian packages (`src/sonic-p4rt/Makefile`)

For legacy package/Make builds, `src/sonic-p4rt/Makefile` runs Bazel 8.5.1
(`USE_BAZEL_VERSION=8.5.1`) inside `./sonic-pins` using an isolated server
(`tools/bazel/private_server.sh`) to build `//p4rt_app:p4rt_deb` and
`//p4rt_app:p4rt_dbg_deb`, run `//p4rt_app/...` tests, and stage the resulting
`.deb` archives into `$(DEST)`.

- `CONFIGURED_ARCH=arm64` selects `--config=aarch64` (`aarch64_trixie`).
- Optimized binaries (`-c opt`) pair GNU `.build-id` split debug symbols from
  `:p4rt_runtime.debug_symbols` (`sonic_deploy_tar`) into `sonic-p4rt-dbgsym`.
- `--experimental_downloader_config=$(CURDIR)/source_mirrors.cfg` retains
  mirrored source-download policy checks.
- If `BAZEL_CACHE` (default `/bazel`) exists, `Makefile` reuses
  `$(BAZEL_CACHE)/cache` and `$(BAZEL_CACHE)/disk_cache`.

For example, the VS DEB workflow builds both
`target/debs/trixie/sonic-p4rt_0.0.1_amd64.deb` and
`target/debs/trixie/sonic-p4rt-dbgsym_0.0.1_amd64.deb`, then verifies split
debug symbols with `tools/bazel/ci/verify_p4rt_debug.py` using `--runtime-deb`
and `--debug-deb` (or `--runtime-tar` and `--debug-tar` for standalone
validation). Both interfaces require `--source-root` and `--output`.

`tools/bazel/tests/p4rt_tar_test.py` checks standalone archive validation and
legacy DEB selection. `p4rt_debug_test.py` compiles native ELF/DWP fixtures and
checks isolated GDB lookup, including invalid and unrelated symbols.

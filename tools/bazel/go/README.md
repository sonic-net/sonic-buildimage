# Go module download cache

`MODULE.bazel` selects the published Gazelle ZIP-cache extension and declares
this root's ZIP lock. The root and CI use the canonical maintained registry
from `securely1g/sonic-bazel-registry/main`.

Update dependencies in their owning Go modules, then run from buildimage:

```sh
tools/bazel/go-lock
tools/bazel/go-lock --check
```

`BAZEL=/path/to/bazel` selects the command used by the entry point and shared
updater. CI uses `tools/bazel/ci/libteam_draft_bazel.sh`, which selects the
maintained registry and preserves the shared CI configuration.
The shared updater in `sonic-build-infra/go/lock.py` uses the rules_go SDK
selected by this entry. Review and commit Go declarations and ZIP locks
together. Ordinary builds consume reviewed digests; missing entries fail with
an update instruction.

Every additional isolated extension needs an imported plan and an additional
`--plan` argument. Explicit `go_repository` declarations with `zip_sums` export
plans through the same interface. The shared updater does not scan Starlark.

Bazel module resolution, source archive caching, and compiled-action caching
remain separate. Module ZIP caching requires `--repository_cache` and/or
`--experimental_remote_downloader`; `--remote_cache` alone caches build actions.
The published Gazelle extension uses the first HTTP(S) entry in `GOPROXY` and
reports unsupported fallback after a download failure. See the shared updater's
README for private modules, proxy policy and failure behavior.

The independent Lemming checkout owns a separate 200-module ZIP lock. Update
that graph separately with the Bazel version selected by its workspace:

```sh
tools/bazel/go-lock --workspace "$PWD/platform/alpinevs/src/libsai-grpc/lemming"
```

That nested source still selects its own temporary registry catalog. Refreshing
its source and Alpine's gitlink belongs to those source repositories; the root
registry configuration does not override the nested workspace.

Run Common's source-owned test directly as
`bazel test @sonic_swss//crates/countersyncd:common_rust_test`.
Bazel builds compatibility aliases without selecting their tests for execution.

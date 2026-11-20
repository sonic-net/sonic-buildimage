# P4RT source images

The AMD64/Trixie VS path builds P4RT and the project components in its
config-engine parent from source. It consumes runtime and debug TARs directly;
it does not generate Debian packages. Locked Debian packages supply external
runtime dependencies and debug tools.

P4RT still owns a Bazel 6.4.0 WORKSPACE. The image and Common use Bazel 8.5.1.
The explicit preparation step builds Common's SDK in the image's dependency
graph, verifies its library is byte-identical to the config-engine runtime,
and imports the libnl owner's existing SDK TARs. All four libnl libraries and
SONAME links must match that same parent; the P4RT link uses those declared
libraries instead of the slave's incompatible libnl ABI. Duplicate SDK headers
must agree, and the source targets and file hashes are retained in the SDK receipt.
Preparation then asks P4RT's source owner to build its five production programs and matching
DWP files. The owner seals the source revision, SDK inventory and TAR hashes.
A standard `local_repository` exposes those files to the image build.

The runtime image preserves the original `start.sh`, `p4rt.sh`, ConfigDB
template, Supervisor configuration and critical-process list. Its debug image
extends that exact runtime with matching symbols, the standard tools and the
source-built FIPS OpenSSH client. Each package-state stage has its own APT
decision bound to the completed base image.

The debug image also consumes Sairedis's library-only runtime metadata and its
existing `libsairedis`, `libsaimeta` and `libsaimetadata` runtime/symbol TARs.
These preserve the legacy debug dependencies. Validation binds their package
records to the exact owner TAR hashes and checks their symbols in the loaded
debug container.

## Build

Use a native AMD64/Trixie build environment with Bazelisk and the P4RT execution
dependencies installed by `bazel-oci.yml`. Initialize the committed source owner:

```sh
git submodule update --init -- src/sonic-p4rt/sonic-pins
export BAZEL="$PWD/tools/bazel/ci/libteam_draft_bazel.sh"
owner_revision=$(git rev-parse HEAD:src/sonic-p4rt/sonic-pins)

for phase in prepare-owner build; do
  python3 -B tools/bazel/ci/p4rt_image.py "$phase" \
    --directory artifacts/p4rt --bazel "$BAZEL" \
    --owner-source src/sonic-p4rt/sonic-pins --owner-revision "$owner_revision" \
    --owner-output-base "$HOME/.cache/sonic-p4rt-owner/manual" --jobs 2
done
```

The resulting `docker-sonic-p4rt.gz` and `docker-sonic-p4rt-dbg.gz` retain their
existing Docker-save format and `:latest` tags. To build through Make, select
`BUILD_WITH_BAZEL_WHEN_AVAILABLE=y`, native VS/AMD64/Trixie, `INCLUDE_FIPS=y`,
`ENABLE_ASAN=n` and `DBG_IMAGE_MARK=dbg`, then request either existing archive
target. Make prepares the source TAR handoff once for a parallel request of both
variants and uses the shared Bazel build/query/export bridge. `INCLUDE_P4RT`
continues to decide whether the installer contains the container. Existing
mounts, shutdown order and excluded-profile producers are preserved.

`BAZEL_CONTAINER_CACHE_DIR` and `BAZEL_CONTAINER_ARGS` also apply to the root SDK
build. `P4RT_BAZEL`, `P4RT_BAZEL_OUTPUT_BASE` and `P4RT_BAZEL_JOBS` select the
separate owner client. Keep each output base private to one active client and
retain it for later builds; no clean is needed between image changes.
In dedicated CI the helper stops its idle image Bazel server before the owner
build and test phases. Make can run other producers concurrently, so it keeps
the shared root server alive. The Make SDK build uses the image cache policy.
The Bazel 6 owner reads the slave's repository/disk caches, the PINS source
mirror rules and any selected remote cache. The VS pool's configured remote
downloader is also available. The owner never uploads local action results.

## Validate separately

`p4rt_validate.py validate` checks all three OCI stages, source bytes, package
state, account inheritance, five real DWP pairs and missing-symbol controls,
parent debug symbols, manifests, image configuration and archive tags.
`p4rt_validate.py incremental` changes the image's template and verifies both
archives invalidate while the source TARs and base stay unchanged; restoring
the template must restore the exact image and archive hashes.

`p4rt_image.py prepare-owner-runtime` prepares the explicit private test runtime.
`p4rt_image.py owner-tests` then runs all original P4RT tests and the installed
package test against the same SDK and TAR bytes. These are separate from normal
image production. The loaded-image job uses `p4rt_loaded.py` to check the actual
archives in disposable containers: original startup, ConfigDB-rendered flags,
private Redis, controller arbitration, P4Info verification and rejection of an
unsupported operation. This software check does not establish ASIC programming
or packet forwarding.

The source owner's GCC 14 optimized split-DWARF output has a separately recorded
fresh-compilation reproducibility limitation. Exact unchanged-cache and
configuration edit/restore checks do not claim fresh compiler byte determinism.

In `bazel-oci.yml`, the AMD64 archive job runs in pinned Trixie on the
`bazel-x64`, `vs` pool. It reserves a private scratch directory for the two
output bases and checks that its filesystem has at least 90 GiB free. The PINS
Bazel 6 client reads its own workspace configuration and the selected CI
repository cache, disk cache and source-mirror rules. Trusted same-repository
builds also read the pool's remote cache and downloader. The owner never
uploads local action results. Fork builds use local caches and source mirrors.
The Ubuntu 24.04 job downloads both archives and their evidence from that run,
starts the services and checks them against the tested checkout. The required
AMD64 archive check includes both jobs. ARM64 runs the existing helper checks.
Public artifacts contain the selected build outputs and provenance summaries.

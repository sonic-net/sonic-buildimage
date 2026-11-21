# Build SWSS with Bazel inside a Make VS image

`BUILD_WITH_BAZEL_WHEN_AVAILABLE` is the shared Make switch and defaults to `n`.
With `y`, containers that register Bazel targets for the selected build
configuration use Bazel. Other containers keep their existing Make build.
With `n`, all containers use Make.

SWSS and [syncd-vs](../../../platform/vs/docker-syncd-vs/bazel/README.md) register their supported
configurations independently. For SWSS, on native AMD64 Debian Trixie with
`PLATFORM=vs` and ASAN disabled, the switch makes Bazel compile
SWSS and assemble its runtime and debug OCI images. Other SWSS configurations
continue to use Make, even with the switch set to `y`.
Bazel composes SWSS directly on the source-built config-engine OCI target.
Scapy is built from the selected source by `@scapy//:wheel`; no Make wheel is an
input to either Bazel Orchagent variant. Make generates the container manifests
and coordinates the other containers, host packages, kernel and final
`sonic-vs.img.gz` image. Legacy Make-only builds retain their Scapy wheel
dependency and metadata.

The source and toolchain revisions are declared in the root `MODULE.bazel`; they do not change
the source revisions used by Make's existing submodules. SWSS and its declared
source dependencies produce runtime tar layers and matching debug symbols.
Pinned Debian packages supply external runtime dependencies; Bazel does not
create Debian packages in this path.

The source runtime added over config-engine now comes from
`//dockers/docker-swss-layer-trixie:source_runtime`. It includes the patched
libteam and libteamdctl used by SWSS's source link; the Orchagent APT roots no
longer request their distribution replacements. This source-only TAR is shared
with the new SWSS parent without changing Orchagent's config-engine selector.

Make selects the builder before starting the build. Missing or invalid metadata
for a selected Bazel target, a missing Bazel executable, or a Bazel build failure
fails the build; it does not trigger a retry with Make.

## Automatically retain base packages in APT layers

Runtime and debug package files use the shared `apt_layer` rule from
`sonic-build-infra`. Pass the full checked dependency set; the selector removes
packages already supplied by the actual base image. No separate list of missing
transitive dependencies needs to be maintained in the image BUILD file.

For example, the snapshot's tcpdump 4.99.5-2 pulls in Debian `libssl3t64`
3.5.6-1~deb13u2, while the config-engine base already has
`libssl3t64` 3.5.7-1~deb13u2+fips. The selector finds that installed package name
in the base's `/var/lib/dpkg/status` and skips the Debian OpenSSL payload. It
adds tcpdump and libpcap when missing, preserving the base's `libssl.so.3` and
`libcrypto.so.3`. The retained OpenSSL satisfies tcpdump's >= 3.0.0 requirement
in this tested image.

After selection, the shared helper checks every known package's `Depends` and
`Pre-Depends` against the final package inventory. It uses Debian version order,
alternative dependencies and virtual providers. A compatible base version is
retained even when it differs from the lock; an unsatisfied requirement fails
the build without replacing the base package. The check does not install or
configure packages, establish `Pre-Depends` installation ordering, or evaluate
`Conflicts`/`Breaks`. Loader and installed-image tests remain necessary for runtime
compatibility. Hash, path and library-collision errors still fail the build.

The reviewed runtime roots remain in `orchagent_debian`; four unused Kerberos
administration packages remain omitted. Debug selection projects the seven
Orchagent debug roots (`gdb`, `gdbserver`, `strace`, `vim`, `sshpass`, and the
FIPS OpenSSH client's `libedit2` and `libfido2-1` runtime libraries) from
`sysmgr_debug_debian` via `@orchagent_debug_apt_inputs` and `:debug_lock_view`,
omitting unused `openssh-server` dependencies such as `libwtmpdb0`.

`@sonic_build_infra//apt:apt.lock.json` is the canonical reviewed package
record, including exact source identities and data/control hashes. Normal builds
import this shared owner lock for `orchagent_debian` and derive `:debug_lock_view`
directly from it. The same lock supplies the base and other container
sets, so source packages and OCI images agree on their Debian inputs.

Bazel generates the matching BUILD input labels automatically. The `apt_inputs`
repository rule reads the same lock and writes `apt_inputs.bzl` in the generated
`@orchagent_apt_inputs` repository. The image loads `APT_INPUTS` from there to
reference each candidate's public `:data` and `:control` targets. This step reads
local metadata; Distroless imports the exact locked packages. Bazel tracks
the lock as an input, regenerating the mapping when it changes. No manual
preparation or checked-in copy of `apt_inputs.bzl` is required.

These declarations describe all candidates, not a manually maintained list of
missing packages. The owner test checks the imported runtime/debug package
metadata and generated mapping against the reviewed lock. The suite lives in
`tools/bazel/tests/integration/select_apt_payloads_test.py` and runs with
`bazel test //tools/bazel/tests:select_apt_payloads_test`.

The shared `//tools/bazel/oci:apt_layer.bzl` macro passes these inputs to the
existing name-based selector.
After checking hashes and file overlaps, the selector copies the selected
archives intact into a declared directory. Standard Distroless `flatten` merges
that directory into a layer. The shared Distroless lock importer preserves the
reviewed package URLs, hashes and dependency edges without resolving the
package list again.

The shared `//tools/bazel/oci:apt_selection` library validates the declared
policy, inspects OCI base and candidate files, calls `sonic_apt.selection`, and
stages the selected archives and receipt. Other containers, including syncd-vs,
use that same implementation. Orchagent declares its AMD64/Trixie policy in
`dockers/docker-orchagent/BUILD.bazel`; the macro generates the policy JSON and
selects the shared command-line program. The owner suite checks the actual
`:runtime_layer_policy` output used by the runtime layer.

Orchagent has no Make-produced native DEB handoff: its component payloads come
from source-owned Bazel targets. Its `retained_source = "source"` policy binds
runtime and debug APT selection to `:libteam_source_receipt` so source-built
`libteam5` and `libteamdctl0` retain their owner TARs rather than distribution
replacements. Runtime selection reads the checked config-engine base and writes
the updated package inventory into `:installed_package_state`; debug selection
reads the completed Orchagent runtime and receives `:runtime_package_metadata`
as its declared `base_package_metadata` input, validating the combined runtime
and debug set against the updated dpkg baseline.

The `runtime_apt_selection` and `debug_apt_selection` targets expose receipts with
the base digest, lock/policy hashes, selected and skipped packages, the validated
dependency inventory, duplicates and non-binary path changes. Source payloads, symbols and archive outputs use
the existing owner rules.

Dependency checking was introduced by infrastructure PR #27 and registry PR #45.
[Infrastructure #29](https://github.com/securely1g/sonic-build-infra/pull/29)
and [registry #51](https://github.com/securely1g/sonic-bazel-registry/pull/51) introduced
the declarative `apt_layer` policy input API. The root `MODULE.bazel` now selects
published shared infrastructure through registry `main`; the policy argument
remains a generated build input.

Same-repository PRs and master pushes build the VS installer and run its OCI
validation jobs. For a manual `Bazel OCI` run, select `skip_vs=false` to
include them. Select `enable_debug_layer=true` to check debug companions.
Native AMD64 and ARM64 package tests also run independently.

Generated Bazel files carry an `AUTO-GENERATED. DO NOT EDIT MANUALLY.` header
that names their generator. The package lock and generated JSON inputs/receipts
use `_generated` metadata for the same notice. Keep this notice when refreshing
the lock; its package resolution and extracted-content hashes must be reviewed.
The package request recipe and policy remain handwritten inputs.

To update packages in a separate clean checkout:

1. Update the Orchagent sets in `sonic-build-infra/apt/requests.json` and follow
   that owner's APT refresh procedure. Review the resulting shared
   `apt/apt.lock.json`, including package identities, versions, dependency edges
   and hashes for every affected set.
2. Keep reviewed extracted-content hashes only for unchanged source packages;
   validate and record data/control hashes for changed package inputs.
3. Publish the reviewed infrastructure update and select it in `MODULE.bazel`.
   Bazel imports its packages and generates the BUILD mapping automatically.
   Run the owner tests and rebuild both images, checking the selection receipts
   and installed consumers.

The selector checks `Depends` and `Pre-Depends`, including version requirements,
across the checked base, retained packages and selected APT additions. Source
TARs layered afterwards are outside that selection check. It does not run
package installation scripts, update the inherited dpkg database or regenerate
loader/Python caches. Full image support remains AMD64; native ARM64 helper
tests do not establish a complete ARM64 image.

## Startup configuration

Bazel builds the existing `sonic-cfggen` command from
[sonic-config-engine](../../../src/sonic-config-engine/README.bazel.md), using
the source-owned [sonic-py-common](../../../src/sonic-py-common/README.bazel.md)
library and declared Common/YANG bindings. The `config:docker_init` action runs
the same `sonic-cfggen -a '{"ENABLE_ASAN":"n"}' -t docker-init.j2` operation as
the legacy Dockerfile. Bazel selects its Python interpreter and native libraries
for the execution machine through the shared `sonic_cfggen_template` macro's
execution-tool dependency.

Config-engine and py-common also expose wheel and test targets that can be
invoked directly with Bazel. The Make switch selects container builders; it
does not select those standalone wheel targets.

The action fixes `PLATFORM=sonic-bazel-build` and clears `NAMESPACE_ID` to avoid
inheriting these host environment settings. This template consumes only the
explicit `ENABLE_ASAN` build input. The resulting shell script retains its live
CONFIG_DB, ASIC vendor, namespace, constants and chassis reads. It renders the
remaining configuration templates when the container starts.

Make's existing `generate_manifest` handles SWSS metadata, service discovery
and manifest overlays. SWSS registers separate runtime and debug JSON outputs in
its owning Make rules. `config/BUILD.bazel` declares these files as inputs to the
shared `manifest_labels` actions, which produce the OCI labels.
Native AMD64 and ARM64 CI prepare the same Make metadata, run the real
config-engine command, check failure exits and verify
the generated startup script. This tool coverage does not extend the complete
SWSS image's AMD64-only support boundary.

## Build

CI and local builds use the maintained registry `main` through the
[shared registry configuration](../../../tools/bazel/README.md#registry-selection).

Follow the repository's normal Make setup, initialize its submodules and
configure the VS platform:

```sh
make init
make configure PLATFORM=vs PLATFORM_ARCH=amd64
make target/sonic-vs.img.gz BUILD_WITH_BAZEL_WHEN_AVAILABLE=y
```

The Trixie slave already includes Bazelisk and selects the version recorded in
`.bazelversion`. Inside the Trixie slave, `BAZEL` can select another launcher
and `BAZEL_CONTAINER_ARGS` adds shell-quoted, single-line Bazel options.
`BAZEL_SWSS_ARGS` remains a fallback when the generic variable is unset. Local
cache settings can also go in the ignored `.bazelrc.user`. The same flag works for the ONIE installer target
`target/sonic-vs.bin`.

For SWSS, Make selects Bazel for `target/docker-orchagent.gz` and
`target/docker-orchagent-dbg.gz`, preserving those Docker-save archive names for
the existing image installer. The debug image adds debugging tools and matching
symbols to the exact runtime image layers.

For an individual container, use the same Make option:

```sh
make target/docker-orchagent.gz BUILD_WITH_BAZEL_WHEN_AVAILABLE=y
make target/docker-orchagent-dbg.gz BUILD_WITH_BAZEL_WHEN_AVAILABLE=y
```

These targets first produce Make manifests for
`docker-base-trixie`, `docker-config-engine-trixie` and `docker-orchagent` under
`target/bazel-manifests/`, while building Scapy through its source-owned wheel target. The debug target also prepares the
`docker-orchagent-dbg` manifest, since its image extends the runtime image.
Bazel consumes `//dockers/docker-config-engine-trixie:docker-config-engine-trixie`
directly for the APT selection and SWSS image. Its Docker installer `.gz` remains
an independent output for Make-built child containers and the installer.

For example, changing a config-engine source layer invalidates the dependent
SWSS OCI actions through Bazel's graph. SWSS does not read
`target/docker-config-engine-trixie.gz` or require a prepared `.oci` directory.

With those inputs already available in `target/`, the equivalent Bazel targets are
`//dockers/docker-orchagent:docker-orchagent.gz` and
`//dockers/docker-orchagent:docker-orchagent-dbg.gz`. SWSS declares these explicit
labels and its Make prerequisites in its owning rules. The
[shared Make bridge](../../../tools/bazel/docker.mk) calls Bazel directly to build
the selected target and query its output path.
The shared archive helper publishes the completed archive to `target/` only
after both commands succeed. It preserves the previous archive on failure and
keeps its timestamp when the new bytes are unchanged.

Prepare all owning manifests before invoking Bazel directly:

```sh
make -f tools/bazel/prepare_manifests.mk \
  'MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-orchagent.mk' \
  BUILD_WITH_BAZEL_WHEN_AVAILABLE=y BLDENV=trixie CONFIGURED_PLATFORM=vs \
  CONFIGURED_ARCH=amd64 ENABLE_ASAN=n DBG_IMAGE_MARK=dbg DOCKERS_PATH=dockers
```

The [legacy OCI import helper](../../../tools/bazel/oci/README.md#use-makes-oci-base)
remains available for Make-produced archives containing native OCI entries. It
does not convert Docker-only archives and is not used by this source-built base.

## Reuse local build caches

Make defaults `SONIC_BAZEL_CACHE_SOURCE` to
`$(SONIC_DPKG_CACHE_SOURCE)/bazel`, regardless of whether
`BUILD_WITH_BAZEL_WHEN_AVAILABLE` is `y` or `n`. Override it with another
persistent host directory writable by the builder user. Make creates the
directory and checks writability before starting Docker.

Make mounts that directory at `/bazel_cache` and
[`tools/bazel/slave.bazelrc`](../../../tools/bazel/slave.bazelrc) at
`/etc/bazel.bazelrc`. The system configuration sets the repository cache to
`/bazel_cache/repository_cache` and the disk cache to `/bazel_cache/disk_cache`.
Ordinary Bazel commands inside the builder, including P4RT, inherit these
settings automatically. Bazelisk keeps downloaded Bazel binaries in
`/bazel_cache/bazelisk`. The shared cache survives removal of the builder and can
be reused from a fresh checkout on the same host. Each builder retains its own
output directories, working files and Bazel server; `output_user_root` is not
shared.

Set `SONIC_BAZEL_CACHE_SOURCE=` to omit both this cache mount and the mounted
system configuration. Bazel then runs with its usual local configuration.
For a container-bridge command that needs different cache paths,
`BAZEL_CONTAINER_CACHE_DIR` remains an optional override using the same
`repository_cache/` and `disk_cache/` subdirectories. Make does not export this
override automatically. `BAZEL_SWSS_CACHE_DIR` remains a fallback when the
generic variable is unset. Setting the generic variable to an empty value
suppresses that fallback; it does not disable caches configured in an rc file.

The source CI job also retains the five GitLab module source downloads selected
by gzip: `rules_gzip`, `toolchain_utils`, `ape`, `download_utils`, and
`rules_coreutils`. The small `bazel-repository-inputs` artifact contains about
572 KB. The VS job verifies each BCR SHA512 again before the long Make build and
supplies separate directories through Bazel's standard repeated `--distdir`
option; the upstream archives all have the same filename. This avoids downloading
these sources again from GitLab on the VS host. Bazel still selects each
published module and checks its integrity; no module override or prebuilt
component is introduced.
`tools/bazel/gzip/source_archive.py` rejects a changed resolved module version
until its expected source integrity is refreshed.

The full VS CI job also enables Make's package cache with
`SONIC_DPKG_CACHE_METHOD=rwcache`. Make restores a matching package or container
archive and writes a newly built result on a cache miss. The SWSS Bazel targets
still invoke Bazel to check their declared inputs. The agent stores both caches
outside the per-run checkout:

| Cache | Host path in VS CI |
| --- | --- |
| Bazel downloads and results | `/data/sonic-runner/cache/sonic-buildimage/bazel` |
| Make packages and archives | `/data/sonic-runner/cache/sonic-buildimage/packages` |

To use both caches for a local build, choose persistent, writable directories:

```sh
mkdir -p "$HOME/.cache/sonic-buildimage/packages"
make target/sonic-vs.img.gz BUILD_WITH_BAZEL_WHEN_AVAILABLE=y \
  SONIC_BAZEL_CACHE_SOURCE="$HOME/.cache/sonic-buildimage/bazel" \
  SONIC_DPKG_CACHE_METHOD=rwcache \
  SONIC_DPKG_CACHE_SOURCE="$HOME/.cache/sonic-buildimage/packages"
```

Omit the `SONIC_BAZEL_CACHE_SOURCE` override to use the default `bazel/`
subdirectory under the selected Make package-cache directory instead.

Bazel uses its declared action inputs to select cached results. Make uses its
existing source, dependency and configuration cache keys. Neither cache freezes
rolling upstream package repositories. CI retains fresh source checkouts and
checks available disk space; it does not delete these caches when a job ends.
Outside the VS workflow, Make's package cache remains opt-in.

## Validation

The VS installer job calls Make to build the installer and its selected OCI
archives in one Bazel batch. The per-container jobs run on `bazel-x64`.
The Orchagent job downloads its own VS archive and build fixtures and exercises
the loaded image, including native library relocation in `tlm_teamd`, Scapy,
configuration files and the installed SWSS services. With debug enabled the
same job checks the companion image, including inherited Common symbols.

A separate job verifies the release and Git commit from the packaged installer.
It passes that release to every container job so each one can verify its image's
release label. AMD64 and ARM64 also run native package and synthetic archive
tests. The required `Container archive (AMD64)` status reports the outcome of
the installer and its dependent checks. See the [shared CI guide](../../../tools/bazel/ci/README.md#shared-helpers).

Each push, manual dispatch and explicit retry runs independently, including
on the same commit. Automatic checks for the same PR and revision share a
group. A new PR revision can run its hosted checks while an earlier VS build
continues. The native VS job has its own worker limit.

The native config-engine base supplies Common's C++ library, Python bindings
and normalized database configuration from the same Bazel-resolved Common
module used to link SWSS. SWSS does not overlay that package again; its debug
image collects the inherited Common symbols through the OCI base dependency.
The selected Common producer already emits
`run/redis/sonic-db/database_config.json`; removing the extra TAR avoids packaging
Common twice. The source-layer guard prevents a regression to older TARs that
used paths beneath `var/run` or other inherited directory aliases.
Complete-image validation remains necessary. Guest
boot, forwarding, ARM64 images and cross builds remain unvalidated. ASAN uses
the existing Make path.

To reproduce the source-layer check in native AMD64 Trixie, install the execution
tools listed in `.github/workflows/bazel-oci.yml`, make the pinned Bazel
version available, and run from a clean checkout:

```sh
for tests in tools/bazel/tests tools/ci/tests tools/ci/runner; do
  python3 -B -m unittest discover -s "$tests" -p '*_test.py' || exit 1
done
make -f tools/bazel/prepare_manifests.mk \
  MANIFEST_METADATA=rules/docker-orchagent.mk \
  BUILD_WITH_BAZEL_WHEN_AVAILABLE=y BLDENV=trixie CONFIGURED_PLATFORM=vs CONFIGURED_ARCH=amd64 \
  ENABLE_ASAN=n DBG_IMAGE_MARK=dbg DOCKERS_PATH=dockers
python3 -B tools/bazel/ci/container.py \
  --container //dockers/docker-orchagent --source-file rules/docker-orchagent.mk --bazel bazel --artifacts artifacts/swss
```

## Change the build

The container's `BUILD.bazel` declares `ci_tests` and `ci_artifacts`. The shared
`tools/bazel/ci/container.py` runner discovers those targets, audits and runs the
suite, and exports the declared files. The shared
`tools/bazel/oci/package_contract.py` implements source install checks from the
owner's generated `:package_policy`. The actual package-validation test in
[`tools/bazel/tests/integration/oci_source_layers_test.py`](../../../tools/bazel/tests/integration/oci_source_layers_test.py)
consumes declared runtime, configuration and symbol archives, original owner
install files and binutils executables. It writes the validation summary as a
Bazel test output. There is no Python CI profile or callback, and the test does not inspect an ambient
external checkout. Make still prepares manifests from its existing declarations.
Test implementations live under `tools/bazel/tests`; manifest conversion and
OCI inventory and package/image regression tests are shared with Syncd.
The policy describes how to read the source install files, required Protobuf
paths, symbol coverage and config modes; it does not copy the source program list.
SWSS owns its opt-in guards, archive
labels and Make prerequisites. It registers both runtime and debug archives in
`SONIC_BAZEL_SWITCHABLE_IMAGES` even when the selector is off; the shared Make
rules track and invalidate each archive when its builder changes. The shared Make
handoff is in [tools/bazel/docker.mk](../../../tools/bazel/docker.mk); `slave.mk`
integrates it with the normal Docker and installer target lists. The same include
prepares registered OCI bases and manifests from each owner's declarations.
The container image and debug-symbol targets live in
[the container BUILD file](../BUILD.bazel).
Shared cache/publication helpers, Cargo configuration and validation
code are described in the [Bazel support guide](../../../tools/bazel/README.md). OCI preparation
lives under `tools/bazel/oci`; runner setup and native Make cache tests live under
`tools/ci`.

Another container can opt in through `SONIC_BAZEL_DOCKER_IMAGES` and
`SONIC_BAZEL_DBG_DOCKER_IMAGES` with its own `_BAZEL_TARGET`, `_PATH` and optional
`_BAZEL_DEPENDS` declarations. Consume Bazel-built bases directly by their OCI
target labels. Register native OCI imports from Make archives through
`SONIC_BAZEL_OCI_BASES` and per-base `_OCI_ARCHIVE`/`_OCI_PLATFORM` values. Register
manifests using `SONIC_BAZEL_MANIFESTS` and each key's
`_MANIFEST_IMAGE`/`_MANIFEST_SUFFIX` values, adding the generated JSON paths to
`_BAZEL_DEPENDS`. See the
[shared bridge example](../../../tools/bazel/README.md#opt-a-container-into-the-make-bridge)
and preserve that container's legacy selectors and metadata. The archive export
recipe, OCI preparation and cache settings are shared; SWSS's package expectations
and supported platform checks remain here.

Edit SWSS source lists, program dependencies and install declarations in the
owning `sonic-swss` repository, then update the pinned source revision here.
Container dependencies belong in `dockers/docker-orchagent/BUILD.bazel`; startup
configuration belongs in `dockers/docker-orchagent/config/BUILD.bazel`. The PR
check compares the packaged programs and data with SWSS's install declarations
and verifies that the container runtime layer preserves them.

`dockers/docker-orchagent/config/BUILD.bazel` supplies SWSS's generated JSON to
the [shared label macro](../../../tools/bazel/oci/README.md) and its non-ASAN
settings to the source-owned `sonic_cfggen_template` macro. Manifest metadata
continues to live in `rules/docker-orchagent.mk`. Other containers reuse the same
Make generator and Bazel label action with their own declarations.

## Source wheel and image acceptance

The Scapy producer is `scapy@2.6.1.sonic.2` on registry `main`. It preserves
SONiC's selected upstream commit, both production patches, the exact
`scapy-2.6.1.dev0-py3-none-any.whl` output, TLS test stability backports and the
upstream build backend. `config/BUILD.bazel` uses the existing standard wheel
installer to add it. The source SWSS TAR is reused from the shared owner;
Orchagent continues to extend config-engine, not the shared SWSS image, and
records its runtime `installed_packages` state plus synthesized `ndisc6`
`traceroute6` alternatives.

The native FIPS debug profile also consumes the existing OpenSSH client TAR
and matching symbols from `sonic-fips`, retains its `_ssh` group and ssh-agent
permissions, and records the installed debug packages. Its pinned external
tools include gdb, gdbserver, strace, sshpass and full vim, matching Make.
These debug additions do not enter the runtime variant.

The existing OCI AMD64 job builds both variants and separately validates
source payload bytes/modes, native link closure, inherited files, matching debug
symbols, the installed wheel and original owner test suites. Its installed-source
edit probe requires both archives to change while preserving compiled symbols,
the config-engine base and Scapy wheel; restoring the input restores digests.
The existing runtime job loads those exact archives and exercises the real
`tunnel_packet_handler.py` with source-built Scapy, a private Redis fixture
and offline IPv4/IPv6 captures. It checks requested ping arguments without
executing them. This does not test ASIC forwarding, live capture, complete
supervisor startup, FIPS POST or a VS installer.

Socat selects [`1.8.0.2.sonic.7`](https://github.com/securely1g/sonic-bazel-registry/tree/main/modules/socat/1.8.0.2.sonic.7).
Its explicit `tar.xz` type accepts the content-addressed Debian patch archive
URL while retaining the original source hashes. The `.3` to `.6` upgrade also
includes the published compiler/debug prefix mapping and compiler-path
normalization from `.4`, plus the `procan` namespace-string termination fix from
`.5`.

Validation uses the maintained registry `main` and BCR through `.bazelrc`. The
FIPS parent retains its selected source and known entropy/assembly-path
reproducibility limitations; same-output-root restoration is not a fresh-root
reproducibility claim.

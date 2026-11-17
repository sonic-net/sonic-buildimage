# Shared Bazel CI helpers

## Remote cache uploads

The [native event services](../../../.github/workflows/bazel-eventd.yml),
[Rust listener](../../../.github/workflows/bazel-supervisor-rust.yml), and
[YANG model distribution](../../../.github/workflows/bazel-yang-models.yml) jobs
use `[self-hosted, linux, bazel-x64]` for AMD64 and
`[self-hosted, linux, bazel-arm64]` for ARM64, matching the cache-reachable agent
pools used by the archive, base-image, and kernel jobs. The agents, including
their job containers where applicable, need network access to
`grpc://172.16.0.4:9092`; public GitHub-hosted runners cannot reach that private
endpoint.

Private remote services are not workspace defaults. Only the five
cache-reachable archive, source-base, event-services, Rust-listener and YANG
jobs opt in with:

```sh
python3 tools/bazel/ci/preserve_setup_bazel_cache.py --private-remote-cache
```

After validating setup-bazel's local cache settings, this appends the private
gRPC action cache and remote downloader to the ignored `.bazelrc.user`, retaining
existing disk/repository caches, source-mirror settings and upload overrides.
The shared `common.bazelrc` imports that file, including for CI wrappers that
ignore the root workspace rc. Without the flag, the helper copies only the
selected local cache settings and never inherits remote settings from the home
rc. Ordinary builds and hosted validation jobs therefore do not require access
to the private cache; existing HTTPS-cache callers can retain their own policy.
Self-hosted status alone does not enable the endpoint. Kernel jobs retain their
explicit `--remote-cache` selection, including the disposable loopback cache
used by cold-cache verification. Make/VS worker pools retain their existing
authenticated cache configuration rather than inheriting the private endpoint.

The remote downloader's local-fallback flag applies to repository downloads;
it does not make an unreachable gRPC action cache safe for ordinary build
actions. Keep both services out of default rc files. On opted-in jobs, a caller
that intentionally disables or replaces the gRPC cache must also disable or
replace its remote downloader.

The [source base-image workflow](../../../.github/workflows/bazel-base-images.yml)
and [OCI archive jobs](../../../.github/workflows/bazel-oci.yml) use these
persistent agents. Neither mounts or deletes host Android, .NET or GHC SDK
directories to reclaim space, nor runs a GitHub-hosted-only cleanup guard; that
cleanup was specific to disposable GitHub-hosted runners. Provision sufficient
build storage and manage host Docker/cache retention outside the job without
deleting active builds.

AMD64 archive jobs upload generic evidence before Sysmgr acceptance, Sysmgr
evidence before SWSS layer acceptance, and SWSS layer evidence before Orchagent
acceptance. Each step preserves its completed artifacts outside the checkout in
a unique private directory under `RUNNER_TEMP`, prefixed with
`pre-sysmgr-artifacts.`, `pre-swss-layer-artifacts.`, or
`pre-orchagent-artifacts.`. This keeps the source-only fence
intact without colliding with or deleting prior jobs' evidence. The preserved
path is logged; retained temporary evidence is subject to agent maintenance.
Sysmgr public evidence is collected only after its build was attempted,
including failed builds, not when an earlier step prevented it from running.

The event-services container reports its identity, cache-directory permissions,
UID/GID mappings and security context before probing Bazel startup. It creates
and removes only a temporary directory in the cache parent, then checks
`bazel info output_base` through the same wrapper used by the build.
Failures stop the job with diagnostics; the probe does not change host
permissions, relocate caches or delete retained build state.

YANG and base-image runtime host jobs restore checkout-directory access before
`actions/checkout`, using the kernel workflow's guarded repair for artifacts
left by container jobs. The repair requires passwordless `sudo`, validates the
expected physical workspace, stays on its filesystem and skips symlinks and
hardlinked files.
It does not delete files or change shared-cache targets. Any repair failure
stops the job before checkout.

Kernel host jobs retain the master's fixed directory names under the short
`KERNEL_RUN_TEMP=/mnt/build/k` root; GitHub's `RUNNER_TEMP` is unchanged. Before
building, [`kernel_scratch.py`](kernel_scratch.py) checks the Speakup path budget,
including the longest header, encoded path bytes and a ten-digit sandbox ID.
The workflow first provisions a missing `/mnt/build/k` using noninteractive sudo.
It requires the physical `/mnt` and `/mnt/build` parents to be root-owned and
not group/world-writable, and checks that the build disk is the same filesystem
as the runner's temp storage. An exclusive `mkdir` creates only the absent `k`
with mode 0700, then `chown --no-dereference` assigns that new directory to the
runner's numeric UID/GID. Existing directories are never chmodded or chowned;
the unprivileged preparation step validates their ownership and runner identity.
Symlinks and non-directory roots are rejected. The agent needs passwordless sudo
permission for these two provisioning commands; failures stop the job. Parent
permissions are unchanged, and no repository Python or kernel build runs under
sudo. There is no fallback to a smaller boot disk.

A retained `.runner-owner` identity binds the root to the runner name and physical
runner temp directory. A second agent cannot reuse that root: provision a separate
short root for it and keep one active job per runner work/temp directory. This is
an operational guard, not isolation from hostile jobs running as the same user.
After preparing the root and before checking its 45 GiB free-space requirement,
cleanup removes only the allowlisted kernel build/verification state, disposable
cache, artifacts and logs. The producer still requires new state directories;
stale state is deleted, not reused. The same cleanup runs after artifact upload
and cache-server shutdown, retaining the root and its runner identity.
It validates all targets before deleting any, rejects workspace overlap,
redirected paths and nested mounts, and preserves shared-cache symlink targets
and hardlinked file metadata. An existing live cache PID blocks cleanup rather
than killing a possibly unrelated process. Cleanup errors fail the job; a cache
shutdown failure preserves state. Cancelled jobs can leave state for the next job's cleanup;
old `kernel-job.*` directories still require periodic agent maintenance.
Bazel's default startup location, cache policy and kernel build settings are
unchanged. Compared with the observed 36-byte runner temp prefix, the short root
saves 24 bytes. The longest Speakup input is 238 bytes with the observed sandbox
ID, or 245 bytes with the reserved ten-digit ID, below its 255-byte limit.

Base-image, event-services, Rust supervisor and OCI archive container jobs keep
Bazel's default `/github/home/.cache/bazel/_bazel_root` startup directory.
After cache setup, [`repair_bazel_startup.py`](repair_bazel_startup.py) restores
ownership of directories in that tree to the root container user. Bazel requires
the startup directory's owner to match its effective UID even when the process
can write there. The helper requires GitHub Actions, the expected container home
and a root job container. Directory descriptors prevent following symlinks, and
mount checks exclude redirected startup paths and nested mounts. Regular files,
hardlinks and repository/disk caches are not modified by ownership repair;
failures stop the job.
No startup path override is exported. The shared wrapper still accepts explicit
caller startup options for isolated cache-verification runs.

The helper also verifies a batch Bazel startup without loading rc files. If
Bazel reports a corrupt extracted installation, it validates the reported
32-character hash directory under the default `install` tree, removes only that
directory without following symlinks or crossing mounts, and retries startup
once so Bazel re-extracts its embedded files. Other installation directories,
output bases and repository/disk caches are retained. Other startup failures
and a failed recovery preserve Bazel's exit code and stop the job; installation
timestamps are never patched to bypass Bazel's validation.

The Bazel OCI workflow enables remote action-cache uploads for the AMD64 and
ARM64 archive builds and VS builds only on pushes to `master`. Pull requests
and manual runs remain read-only. The archive job overrides the default in its
ignored `.bazelrc.user`, which the shared Bazel configuration imports without
modifying tracked files. The active `vs_container` job and the disabled legacy
`vs` job forward the same policy through `BAZEL_CONTAINER_ARGS`. The specialized
upstream source-image and VS slave worker pools retain their existing selectors.

The agents and builder containers must be able to reach the remote cache, and
the server must authorize writes. Enforce writer permissions on the server:
the workflow flag is not an authorization boundary for pull-request code.
Enabling uploads does not guarantee backfilling results already present in
local caches.

The fresh-checkout disk-cache verifier explicitly disables the remote cache,
remote executor, and remote downloader for its isolated builds. This keeps remote
hits from masking disk-cache misses and avoids enabling the remote downloader
without the gRPC cache it requires. Normal workflow builds retain their configured
remote services.

If an isolated build fails, the verifier prints its retained Bazel build log
and exit code to the CI console before raising the failure. The local
`agent-cache/<phase>/build.log` remains available on the runner; raw logs are
not added to public failure artifacts.

## Shared helpers

These helpers validate buildimage artifacts and preserve build evidence. The
shared container runner discovers the selected owner's Bazel CI targets;
package expectations remain in the owner declarations and tests.

[`.github/workflows/bazel-oci.yml`](../../../.github/workflows/bazel-oci.yml) is
the shared entrypoint for Bazel OCI container CI. It runs common AMD64/ARM64
archive checks, lightweight hosted regressions and the full VS integration
job. The active `vs_container` job builds inside a privileged SONiC slave,
then runs shared SWSS/Syncd static image checks and Docker-load validation in
that same worker. Build and validation share its Bazel server and caches.
The older `vs` job is disabled; its post-Make source-package suites and
cache-action reports remain in that legacy path and are not run by the active
job. Add checks for future
OCI containers in this workflow, keeping their target lists and package contracts with each container.
Keep checks that compile image inputs in the full build job so they can reuse
its action outputs.
The owning BUILD files sit beside the original container inputs:
[`dockers/docker-orchagent/BUILD.bazel`](../../../dockers/docker-orchagent/BUILD.bazel)
and [`platform/vs/docker-syncd-vs/BUILD.bazel`](../../../platform/vs/docker-syncd-vs/BUILD.bazel).
The shared runner accepts either package path; it does not require owners to
live under `dockers/`.
Manual runs default to `skip_vs=true`; same-repository PRs retain the full VS
job after hosted regressions and dependency resolution pass. Fork PRs and
manual `skip_vs=true` runs omit source-package/full-image validation; hosted
success alone does not establish those results.

SWSS compilation, package contents and component tests are validated in the
[SWSS source repository](https://github.com/securely1g/sonic-swss). Buildimage
consumes those targets and validates the assembled OCI images, including their
installed behavior and image-cache invalidation.

The native VS Make build produces the runtime and debug archives together for
each registered Bazel container, using the same builder and Bazel invocation
for each pair. The normal installer still selects runtime containers;
`INSTALL_DEBUG_TOOLS=y` selects debug containers. Producing a debug archive does
not add it to the installer.

The active **VS installer in SONiC slave (AMD64)** job uses a host agent and
the native Make wrapper to build `target/sonic-vs.bin`. After managed-identity
registry login, Make reuses a local computed-tag slave image, pulls the matching
tag if absent, or builds locally if the pull fails. The separate
[slave publication workflow](../../../.github/workflows/sonic-slave-images.yml)
publishes generic AMD64 Bookworm and Trixie images on master pushes using the
Azure slave pipeline's generic build profile, with snapshot mode explicitly enabled.
Both workflows set `MIRROR_SNAPSHOT=y` to use Debian snapshots under
`packages.trafficmanager.net/snapshot`. They retain the repository's base-image
registry and Docker/APT retry defaults, without separate slave-only overrides.
Registry authentication selects the subscription configured in the repository
Actions variable `ACR_SUBSCRIPTION_ID` after managed-identity login.
Both jobs export a private `DOCKER_CONFIG` before Docker operations and always
attempt registry logout and credential-file removal at job completion. They also
prune unused Docker images and build cache before checking free space, so their
agents must not share a Docker daemon with concurrent jobs.
It does not update image pins or create commits. See the
[host-agent image policy](../../ci/runner/container-vs.md#slave-image-publication-and-selection).
The installer job checks the installer version and retains runtime/debug archive
identities across validation, using the shared `validate_runtime.py` validator.

The host wrapper [`build_vs.sh`](build_vs.sh) runs one
`make target/sonic-vs.bin` request
with the native AMD64 Trixie VS configuration, Bazel, FIPS and DASH enabled.
The **Build the VS installer (.bin)** workflow step ends
when Make finishes producing the ONIE installer and its runtime/debug archive pairs.
It does not request those archives again or run the post-build validators.
This target does not require KVM. BIOS/UEFI VM disk generation and guest boot
checks belong to `target/sonic-vs.img.gz` and are not exercised by this `.bin`
CI path. The separate VM target retains those checks.
The build records its starting Git revision; validation requires that revision
before checks and again before writing its timing summary.

[`validate_vs.sh`](validate_vs.sh) runs in the separate **Validate VS container
pairs and P4 debug symbols** step after a successful build, using the same
checkout and configuration. It first checks
both Orchagent and Syncd pairs before another builder can produce a missing
archive. The checks cover Linux/AMD64 configuration, Make manifests, Bazel
producer stamps and each debug image's runtime layers. They also check that
`target/sonic-vs.bin.dockers`, emitted from the installer's actual inputs,
selects runtime Orchagent and Syncd rather than debug archives for this normal
VS configuration. Installed-container validation remains a separate check of
the resulting container images.

For a separate `.img.gz` build, `vs_archive_reuse.py capture-pairs` accepts
`--installer sonic-vs.img.gz`; verification follows the image recorded in the
receipt. Older receipts without an image name retain their `.img.gz` meaning.

The validator prepares Bookworm through `Makefile.work` unless `NOBOOKWORM=1`,
then uses its existing `sonic-slave-run` entrypoint to run `verify_vs_oci.sh`
in Trixie. Make expands
`SONIC_BUILD_INSTRUCTION` for that command so it retains the selected build
configuration. The verifier queries Make for the resolved profile and existing
P4RT runtime/debug package names rather than hardcoding package versions.
It requires native AMD64/Trixie VS with Bazel, FIPS and DASH enabled, without
ASAN, Syncd RPC or cross/emulated execution, and checks that both runtime/debug
container pairs select their Bazel producers.

The shared verifier resolves both P4RT packages through Make. Native CI uses
the config-engine archive already produced for the installer. Image builds
and validation use the workspace's normal Bazel server and output links,
without per-image output bases or shutdown hooks. SWSS and Syncd checks run
sequentially on that shared server, selecting DASH only for Syncd. Standalone
validation can request the config-engine archive through Make. The offline
P4RT split-debug-symbol check runs in the same Trixie builder.
It does not request the Orchagent or Syncd Make archives again or add a package
definition. The legacy wrapper's Bookworm builder cleanup
is retained. The final pair check requires all four archives and their manifests
to remain unchanged. Missing, changed or incompatible outputs fail validation;
failures are not hidden by rebuilding an archive.

The P4 image test still declares the native Syncd OCI images and Bazel archive
outputs as inputs and compares them with the retained Make archives. Bazel may
restore those inputs or rebuild them on a cache miss in the validation builder.
The native build and validation steps share persistent action/download caches.
P4RT and kernel workspaces keep their separate launchers.
Splitting the workflow steps does not remove this work or
claim a speedup. The active image checks do not replace the source-package
suites retained in the disabled legacy job. Neither path's installer checks
establish BIOS/UEFI VM boot success.

Each container retains its owner's dependencies and Bazel options. Standalone
archive requests and other configurations retain their existing rules.
Validation stops on a failed prerequisite or check.

[`verify_vs_oci.sh`](verify_vs_oci.sh) runs the uncached SWSS and P4 image tests and offline
split-debug package check before the validation builder exits. It preserves
`artifacts/vs/swss-images/`, `swss-images.log`,
`artifacts/vs/syncd-p4-images/`, `syncd-p4-images.log`,
`p4rt-debug-verification.log` and `p4rt-debug-verification.json`. The archive
checker retains `runtime-debug-pairs.json` and `orchagent-reuse.json`; these
are validation receipts, not inputs needed to assemble the image.
Each family's test logs, XML and static receipt are copied before switching
Bazel configurations. The generated `*-installed-images-policy.json` files
bind the Docker checks to those outputs; `installed-images.json` records the
shared Docker-load result. The active workflow publishes these files in
`sonic-vs-image-validation-amd64`.
The Syncd `validation.json` includes additive `performance` evidence:
`phase_seconds` separates input identity, image/archive validation, native
packages/symbols, inherited base/symbols, and final input-integrity verification.
`validation_seconds` excludes Bazel dependency preparation. Progress and timings
are printed in the test log. Scoped cache statistics count hash reads/hits and
bytes, inventory hits/misses, evictions and size/file-limit bypasses, plus
separate final-integrity reads, bytes and duration.
Successful duplicate-path checks for identical OCI layer bytes are reused only
within the immutable-input cache scope. Invalid layers are never cached, and
final byte-integrity verification remains mandatory.
`runtime-debug-pairs-timing.json` (schema 2) records the revision and profile,
P4 validation time, pair validation time and their total. It does not include
the image build duration.
The successful image artifact contains `target/sonic-vs.bin` and the four
Orchagent/Syncd runtime/debug archives. The recorded installer selection is
`target/sonic-vs.bin.dockers`.
Do not parallelize top-level Make goals that share Docker state.

GitHub Actions reports build and validation as separate step durations. Compare
the complete job, including later source checks and artifact uploads, when
measuring an improvement. Moving validation out of the build step changes the
timing boundary, not the total work. The `.bin` installer step also has a distinct
dashboard measurement from historical `.img.gz` builds: omitting VM generation
and boot checks is a change in scope, not a same-scope build speedup.
Helper tests exercise dispatch, ordering, configuration and failure behavior
without Docker; full VS CI is needed to
validate the resulting image and measure end-to-end time.

- `container.py` runs an owner's `:ci_tests` test suite and builds its
  `:ci_artifacts` filegroup. Tests and exported files are declared in the
  container's `BUILD.bazel`; no Python CI configuration or callback is loaded.
  For example, for separate cold runs after preparing the matching Make
  manifests (use separate clean workspaces for these examples):

  ```sh
  python3 -B tools/bazel/ci/container.py \
    --container //dockers/docker-orchagent --source-file rules/docker-orchagent.mk --bazel bazel --artifacts artifacts/swss
  python3 -B tools/bazel/ci/container.py \
    --container //platform/vs/docker-syncd-vs --source-file platform/vs/docker-syncd-vs.mk \
    --bazel-arg=--config=syncd-dash --bazel bazel --artifacts artifacts/syncd-vs/bazel
  ```

  Start from native AMD64 Trixie without `MODULE.bazel.lock` and use an empty
  artifact directory. To validate after Make, pass `--reuse-build`: it requires
  the existing nonempty, ignored and untracked generated lock, records its
  initial hash, and retains the tested lock/graph afterward. It never deletes
  the lock, clears outputs or disables package/source checks. Use Make's exact
  feature flags (`--config=syncd-dash` for Syncd; SWSS always inherits the
  source-built config-engine image), both native platform flags,
  and the same cache/download options. In this mode, the Git cleanliness check
  covers the declared Bazel inputs and their containing submodules, allowing
  unrelated legacy Make submodules to create their normal build patches.
  Changes to validated inputs or their submodule commits still fail.
  In reuse mode the artifact group is built before any tests, using Bazel's
  execution log to record cache hits and executed actions by target and mnemonic.
  The raw log is private and deleted after extracting those counts; no command,
  environment or file-path fields are published. This distinguishes reuse of
  Make's production outputs from later test-only compilation. Cache misses are
  reported for inspection rather than hidden or treated as a test failure.
  Reuse Make's generated manifests rather
  than rerendering them with different defaults. The runner expands nested test
  suites, audits their actual targets and the artifact group before execution, runs uncached tests, collects
  configured logs/XML and package-check summaries, and exports the group's files
  under their declared basenames. Duplicate output filenames fail; an empty
  artifact group is valid for contract-only CI. Dependency resolution and source
  hashes are retained with success/failure receipts. `--bazel-arg=...` appends an
  explicit build option override. `--source-file PATH` adds a workspace-relative
  preparation input to the source evidence; the workflow supplies the same
  owning Make include that prepares the manifests.

  SWSS's actual source-layer validation is a Bazel test with declared archives,
  owner install files and binutils tools. Syncd's suite validates its existing
  Make/source package adapter. Both suites use the same manifest-test program
  and the shared OCI package/image regression tests. Adding a test or artifact
  requires changing the owning BUILD declaration, with no second list in the
  runner or public artifact test-count checks.
- `artifact_validation.py` streams tar contents to record ownership, modes, file
  hashes and little-endian ELF64 metadata, and checks matching split debug symbols.
  SWSS and syncd share its ELF inspection and debug-link checks; each consumer
  supplies its package inventory and platform policy. SWSS's inventory and
  Protobuf expectations are declared in its BUILD-generated `:package_policy`
  and interpreted by `tools/bazel/oci/package_contract.py`.
  `debug_archives` combines a runtime tar and its symbols tar, rejects payload
  collisions and foreign debug paths, checks the caller's ELF architecture,
  safely extracts the pair and verifies matching build IDs, DWARF and debug links.
- `validation_inputs.py` shares SHA256 digests and successful archive inventories
  during one `syncd_p4_images_test` process. Image, native-package and inherited
  source checks can read the same runtime/debug layers without repeatedly
  decompressing and hashing their contents. Each retained input path is hashed
  before reuse by content, and its device, inode, size, mode, mtime and ctime are
  checked around reads and at reuse. A final identity and SHA256 check runs before
  publishing the validation report. Changed retained inputs fail validation.
  Only the test's declared files and OCI directories qualify; extracted inspection
  files remain ordinary uncached reads. The cache retains at most 64 MiB of
  inventory metadata and 512 file identities; larger inputs fall back to ordinary
  reads. It retains no extracted payload bytes and nothing persists between test processes.
  Inventories are copied before use. For identical archive contents and the
  same parsing rules, reuse includes normalization and checks made during the
  scan, such as member type and ownership checks. Checks against each caller's
  current overlay and receipts, plus ELF/debug inspection, still run each time.
- `build.py` builds a caller's filename-to-label mapping and collects one nonempty
  archive per label, with atomic publication, command logs, sizes and hashes.
  `collect_artifacts` exports all files from a declared CI filegroup. It checks
  every output before publication and records each file's size, hash and Bazel
  output path. Owner package checks run as Bazel tests with declared source inputs.
  A package test records every validated archive's basename and SHA256 in
  `validation.artifact_sha256`. The runner and public upload collector require
  the exported names and bytes to match. The source-layer test and artifact filegroup use identical build options
  so their inputs and exported bytes match.
- `command_log.py` captures command output and timings while keeping diagnostic
  messages separate from queried artifact paths. Callers supply the working
  directory and evidence directory. The shared runner sends raw action-graph
  stdout to a temporary private file, retains only stderr in the log and removes
  the raw graph after its execution audit, including on failure.
- `public_artifacts.py` prepares explicit public upload lists for the archive,
  SWSS source, VS and Syncd contract evidence. The three source/image artifact
  selections now run in the VS job; the successful VS summary also requires
  matching successful SWSS and Syncd summaries. Container jobs compare the expanded
  suite identities with successful build-event results instead of maintaining
  a second fixed test count. Successful jobs retain their expected
  build outputs and validated dependency JSON, plus summaries containing only
  declared status, count, revision and hash fields. Failed jobs may retain only a
  partial summary. The helper excludes raw command, Bazel event/execution and
  test logs/XML from uploads; these records remain in the job workspace.
  It rejects missing successful outputs, symlinked inputs, nonempty recorded
  environments, diagnostic command fields, credential-bearing URLs, common token
  formats and known host build or
  cache paths. The owning build step remains responsible for validating binary
  payloads. Update this helper's declared paths, test counts and receipt fields
  with the owning workflow when its outputs change. The helper does not run a
  build, and its direct tests use synthetic records only.
- `python_packages.py` exports the config-engine and py-common wheels and retains
  source-suite logs/XML, collected-test inventories and the installed-wheel
  receipt. It requires matching wheel hashes, native architecture and Python
  version, and fails when any required evidence is missing.
- `resolution.py` retains the generated module lock and resolved graph. Its
  explicit allowlist for known unused-extension failures is tied to the selected
  dependency versions and rejects incomplete graphs or unexpected failures.
- `verify_agent_cache.py` builds the shared OCI archive fixture across fresh
  checkouts and output bases, verifies disk-cache hits, then changes an input and
  requires a rebuild. It shares cache settings through `tools/bazel/build_helpers.py`.

Run the helper tests from the repository root:

```sh
python3 -B -m unittest discover -s tools/bazel/tests -p '*_test.py'
```

Native Debian package analysis also requires `file`, `readelf` (from
`binutils`) and `pkg-config` on the host running Bazel. `file` distinguishes GNU ld
linker scripts from ELF libraries; `pkg-config` resolves the declared package
interfaces. The source-base and Rust listener CI containers install these tools
for both AMD64 and ARM64 before invoking Bazel.

The ELF tests use `cc`, `readelf` and `objcopy`. The cache integration check uses
the repository's selected Bazel version and platform; its artifact directory
must be new:

```sh
python3 -B tools/bazel/ci/verify_agent_cache.py \
  --bazel bazel --artifacts artifacts/agent-cache
```

The workspace rc uses the [maintained registry selection](../README.md#registry-selection).
Resolution receipts retain the actual registry URLs from the generated lock
instead of inferring them from the default `.bazelrc`.

## Public build evidence

Container CI uses `public_artifacts.py` from the public upload work in
[Image12](https://github.com/securely1g/sonic-buildimage/pull/12). It uploads
explicit build outputs, checked dependency records, and summaries of build and
test results. Raw Bazel events, command lines, logs, and receipt diagnostics stay
out of public artifacts. A failed job publishes only a validated partial summary.
The selector fails closed for unsafe dependency data or missing successful outputs.

## Debian download mirrors

The root `.bazelrc` selects [`debian_mirrors.cfg`](../debian_mirrors.cfg) for
Bazel-managed dependency downloads, including image packages and published
toolchain inputs. The pinned upstream Debian snapshot `20260727T143429Z` maps
to the SONiC mirror snapshot `20260727T000818Z`; security snapshot
`20260726T121236Z` maps to `20260727T000824Z`. Original upstream URLs remain
fallback candidates. Existing image-lock mirror URLs are normalized to HTTPS,
and their security snapshot `20260726T000750Z` maps to `20260727T000824Z`.
Other snapshot timestamps retain their upstream location.

The policy changes download locations, not generated locks, package versions
or checksum verification. When advancing snapshot inputs, verify the matching
mirror content before updating the mappings. Commands that ignore rc files
must pass `--downloader_config` explicitly. The policy does not rewrite
`apt-get` or arbitrary `curl` commands, and does not provide connectivity to the
private remote cache/downloader. The mirror may redirect HTTPS requests to
HTTP, so HTTPS source URLs alone do not guarantee end-to-end TLS.

The source-base, OCI archive, and full VS jobs also use the registry's shared
[`ci/source_mirrors.cfg`](https://github.com/securely1g/sonic-bazel-registry/blob/main/ci/source_mirrors.cfg).
[`source_mirrors.py`](source_mirrors.py) downloads one commit-pinned copy and
verifies its SHA256 before adding it after the local Debian rules. CI selects
the combined file with Bazel's `--downloader_config`, preserving the local
mirror priorities and the normal repository cache. For example, the unchanged
BCR `pigz@2.8.bcr.2` archive comes from the registry release, with MacPorts as a
fallback; Bazel still checks the original SHA256 and applies the BCR patches.
There is no separate pigz download or pigz distdir.

The native K3s installer tries the locked `snapshot.debian.org` URL first for
timestamped Debian and Debian security inputs. The SONiC and registry mirrors
remain available if that snapshot cannot serve the requested bytes.

On native K3s workers, Dockerfile `RUN` steps inherit the job's managed APT
route for `deb.debian.org` when that route is enabled. BuildKit mounts the APT
configuration and its public CA for each step. The native preparation receipt
records the selection in `docker_apt_proxy_enabled`.

The private source-image worker uses the same verified shared configuration
and retains its additional exact gawk and snapshot fallbacks. Its private
cache paths and download artifacts are unchanged.

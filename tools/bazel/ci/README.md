# Shared Bazel CI helpers

## Unsigned VS ONIE payload streaming

The existing `SONIC_VS_FAST_ASSEMBLY=1` profile enables
`SONIC_VS_STREAM_ONIE=1`; set `SONIC_VS_STREAM_ONIE=0` to retain the original
wrapper. Without the profile or explicit flag, behavior is unchanged.
Streaming applies only to native AMD64 VS OS installers using
`DOCKER_LOAD_ON_BOOT=y`, with unsigned mode `none`, empty or `no_sign`.
Signed, cross-built, other-platform and legacy Docker-store installers retain
their original wrapping path.

The wrapper keeps its normal private staging copies, then writes the tar
payload once while computing its SHA1 checksum and byte count. It backfills
a 4096-byte, comment-padded shell header and atomically publishes the completed
image. The existing `exit_marker` reader, tar metadata, embedded/sidecar bundle
handling and source-file ownership remain unchanged. The streaming path
expands the stock `TARGET_MACHINE` raw-image placeholder without shell evaluation;
other shell substitutions require the original wrapper.

This removes the intermediate tar, checksum reread and final tar-to-image copy.
It does not change package registrations, filesystem scheduling, bundle
contents, signing or shared cache permissions.

## Transient repository fetch retries

The shared action-audit query, archive/filegroup collectors and container
action query opt into `command_log.execute(..., retry_fetch=True)`.
They retry explicit Go `fetch_repo` or Bazel `Error downloading` transport
failures (including EOF, reset connections and timeouts), and temporary HTTP
429/500/502/503/504 failures. There are at most three attempts, with 10s and
30s backoff, using the same command, working directory and Bazel caches.
This covers dependency fetching during `aquery`, not just the later build.

Only exit code 1 with recognized fetch diagnostics is eligible. Unrelated
error diagnostics, checksum/certificate failures, unknown versions, missing
packages and cancellation are not retried. Ordinary commands remain
single-attempt unless a Bazel caller explicitly opts in. Successful queries
return only the final attempt's stdout, and private action graphs are truncated
before each attempt and never printed or logged.

The normal command receipt still has one entry and the final `<name>.log`.
Retried entries additionally record every attempt's exit status, elapsed time,
log and backoff; failed attempts use `<name>.attempt-N.log`. Total command
wall time includes all attempts and waits. Standalone action audits retain a
neighboring `<audit-name>.query.json` command receipt even on fetch failure.
The action-graph policy check still runs after a successful query; retries
cannot bypass the no-DEB/package-wrapper gate.

## Remote cache uploads

The [native event services](../../../.github/workflows/bazel-eventd.yml),
[Rust listener](../../../.github/workflows/bazel-supervisor-rust.yml), and
[YANG model distribution](../../../.github/workflows/bazel-yang-models.yml) jobs
use `[self-hosted, linux, bazel-x64]` for AMD64 and
`[self-hosted, linux, bazel-arm64]` for ARM64, matching the cache-reachable agent
pools used by package and kernel jobs. The agents, including
their job containers where applicable, need network access to
`grpc://172.16.0.4:9092`; public GitHub-hosted runners cannot reach that private
endpoint.

The package, event-services, Rust-listener and YANG jobs opt into private remote services with

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

The [Bazel OCI workflow](../../../.github/workflows/bazel-oci.yml) publishes
the AMD64 VS build once. The container validation jobs consume its artifacts on
host agents labeled `bazel-x64`. They require Docker and access to GitHub
Actions artifacts. The Teamd probe also uses the host team kernel module.
Component package tests run natively on AMD64 and ARM64.

The event-services container reports its identity, cache-directory permissions,
UID/GID mappings and security context before probing Bazel startup. It creates
and removes only a temporary directory in the cache parent, then checks
`bazel info output_base` through the same wrapper used by the build.
Failures stop the job with diagnostics; the probe does not change host
permissions, relocate caches or delete retained build state.

YANG host jobs restore checkout-directory access before
`actions/checkout`, using the kernel workflow's guarded repair for artifacts
left by container jobs. The repair requires passwordless `sudo`, validates the
expected physical workspace, stays on its filesystem and skips symlinks and
hardlinked files.
It does not delete files or change shared-cache targets. Any repair failure
stops the job before checkout.

### Post-job Bazel disk cleanup

Before source checks, the native Bazel workflows restore already-populated
submodules to their superproject's recorded commits, recursively, then initialize
the required Sysmgr gNOI source. A checkout without `submodules` does not update
retained submodule revisions on a reused runner. The preparation uses
`submodule foreach` to avoid cloning unrelated uninitialized modules, including
registered modules whose working directories are absent. Git fetches missing
pinned commits as needed; conflicting local edits fail explicitly rather than
being forcibly reset. Repository-wide tracked-source checks remain enabled,
including YANG's check. Preparation stays inline so base-image reviews selecting
older source revisions do not depend on a newly added helper in that checkout.

SNMP retains its upload-gated immediate artifact cleanup within the registered
job temporary directory; failed uploads preserve those artifacts. The post-job
hook reclaims the remaining authorized job artifacts and Bazel state.

The [cleanup action](../../../.github/actions/bazel-post-job-cleanup/action.yml)
is registered **before** `setup-bazel` on the AMD64/ARM64 Bazel pools. GitHub
runs post actions in reverse order, so local caches are saved before
[`cleanup_container_bazel.py`](cleanup_container_bazel.py) reclaims disk space.
The final regular step records publication outcomes; it does not delete files.
It runs only after the `bazel_cleanup_register` step succeeds, ensuring Python,
the cleanup controller and the registered job temporary directory are available.
Once registered, publication outcomes are recorded even if a later build fails;
early prerequisite failures or failed/skipped registration skip authorization.

OCI archive, Gearbox, Router Advertiser, base-image, event-services and Rust
supervisor container jobs stop their checkout-owned Bazel servers and remove
only the dedicated `/github/home/.cache/{bazel,bazel-disk,bazel-repo,bazelisk}`
directories. Successful publication authorizes removing checkout artifacts
and the registered per-job temporary directory, including preserved source-only
snapshots. Failed, cancelled or missing publication retains those files with an
explicit warning; local caches can still be reclaimed after cache-save hooks.

The YANG host job removes only its exact checkout/component output bases and
successfully uploaded component artifacts. Shared host repository, disk and
Bazelisk caches, other checkouts' output bases, and Docker-referenced paths are
preserved. Removal rejects symlink roots, changed directory identities and
mounted subtrees rather than following them.
The host runner's work-root alias (for example `_work` pointing to build storage)
is resolved before registration and cleanup. Checkout, temporary and artifact
directories below that physical root still require no-follow traversal; changing
the work-root target invalidates the registered job directory.

Base-image reviews may check out an older source revision. Their small cleanup
controller is staged under ignored `target/ci-bazel-cleanup-controller` before
cache setup, keeping the post hook independent of that checkout.

The general Ubuntu migration opts into the same action with `migrated-job: 'true'`.
Its controller verifies the self-hosted environment and workflow/job allowlist.
Host jobs repair access before checkout; the post hook repairs ownership again
without following links or changing hardlinked files. Root job containers use
the runner-owned repository parent's UID/GID, not container root's identity.
Migrated jobs retain evidence if the job fails, even if an upload succeeded.
Otherwise all declared uploads must have valid successful/skipped outcomes,
and existing generated artifacts require at least one successful upload.
Jobs without generated artifacts or uploads can reclaim their empty registered
temporary directory. Dashboard publication authorizes only `site` and `history`;
other migrated jobs authorize only `artifacts`. These host jobs stop their own
Bazel servers but preserve host output bases and shared caches. Acceptance-only
gates and Pages deployment create no checkout/build state and need no hook.
The mixed validator matrix registers cleanup only for its self-hosted AMD64 leg.

This post-job cleanup does not run on the VS installer agent, delete runner metadata,
touch shared NFS/remote caches or prune Docker images. Self-hosted image-runtime
host jobs separately reuse the VS installer's pre-download Docker image and
builder-cache pruning step. Their VMs must have exclusive use of the daemon,
with no concurrent jobs or background container builds. Container-based build
jobs and GitHub-hosted runtime jobs do not receive that step. No additional
host hook is required; containers and volumes are not pruned.
Agents already unable to
start a job need a separate one-time cleanup while idle; a post hook cannot
recover an agent that fails before checkout. Continue managing host Docker
retention outside active jobs.

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

The DASH HA runtime job binds checkout identity before downloading its build
artifacts. Its strict source check receives the pinned merge's actual base and
PR head from `checkout_identity.py`, not the potentially stale event base.
Archive revision and runtime checkout equality checks remain enabled.

The Bazel OCI workflow enables remote action-cache uploads for the AMD64 and
ARM64 archive builds and VS builds only on pushes to `master`. Pull requests
and manual runs remain read-only. The archive job overrides the default in its
ignored `.bazelrc.user`, which the shared Bazel configuration imports without
modifying tracked files. The active `vs_container` job and the disabled legacy
`vs` job forward the same policy through `BAZEL_CONTAINER_ARGS`. When the
Gearbox archive job uploads, it also sets `--remote_timeout=600s`: it uploads
the 6-9 GB Syncd debug layer and Gearbox debug archive, and the cache does not
resume interrupted writes, so with the 60 s default each upload fails after
exhausting its retries. The specialized
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

The [Bazel OCI workflow](../../../.github/workflows/bazel-oci.yml) builds the
VS installer and its configured OCI archives in the `vs_container` job. Each
container has a separate validation job. That job downloads its runtime archive,
owner fixtures and recorded OCI metadata, checks the archive and runs the service
probe against those exact bytes. When Make enables debug images, a container
with a registered debug target also gets its debug archive checked in the
same job.

The owning BUILD files sit beside the original container inputs:
[`dockers/docker-orchagent/BUILD.bazel`](../../../dockers/docker-orchagent/BUILD.bazel)
and [`platform/vs/docker-syncd-vs/BUILD.bazel`](../../../platform/vs/docker-syncd-vs/BUILD.bazel).
The shared runner accepts either package path; it does not require owners to
live under `dockers/`.
Manual runs default to `skip_vs=true`; same-repository PRs retain the full VS
job after OCI regressions and dependency resolution pass. Fork PRs and
manual `skip_vs=true` runs omit the installer and its dependent validation jobs.

SWSS compilation, package contents and component tests are validated in the
[SWSS source repository](https://github.com/securely1g/sonic-swss). Buildimage
consumes those targets and validates the assembled OCI images, including their
installed behavior.

The native VS Make build submits all selected Bazel container archives in one
invocation. For example, Orchagent and Syncd build together from common base
layers. The Syncd sources read the selected DASH settings. On the build dashboard, `vs-container-images`
records that invocation and its profile. The container rows record their
archive exports. The normal installer selects runtime containers, and
a debug installer requires `INSTALL_DEBUG_TOOLS=y` and `SONIC_ENABLE_DEBUG_LAYER=y`.

The `sonic-vs-bazel-timings-amd64` workflow artifact also includes sanitized
`<owner>/recipe-*/build.bep.json` and `test.bep.json` files for instrumented
Bazel build/test commands. These are JSON-lines BEP event subsets, not the
profile-derived `summary.json`. They retain Bazel version/command, numeric build
metrics, reviewed runner/mnemonic names, target/test results and the original
terminal marker when present. Commands, environment values, progress text,
metadata, local paths, output locations and event-graph links are omitted.
The files are therefore not full BEP streams for graph-replay tools.
Incomplete streams are not marked complete. Invalid or unsafe streams are not
published and produce a fixed warning without changing the build exit status.
Capture reuses an existing caller-owned BEP without modifying it; otherwise
the raw stream is created in a private temporary directory and removed after
sanitization. The nested kernel launcher uses its existing BEP. Capture runs
on success and failure; disabled timing and query-only commands are unchanged.
No execution logging, cache-policy changes or BEP-derived dashboard analysis
is enabled by this artifact publication.

After the native build, `summarize_bep.py` reads those sanitized files once and
adds `bep-summary.json` and `bep-summary.md` to the same artifact. The reports
include per-invocation wall/CPU/analysis/execution timing when available, action
and mnemonic counts, separate local action-cache statistics and runner counts,
and target/test results. They do not infer per-target duration. Phase timings
can overlap and must not be added together. Bazel's `actionsExecuted` includes
cache-handled and internal actions; local cache misses do not establish remote
cache misses. Missing metrics remain unavailable, and missing terminal/result
events produce an incomplete status rather than success.

Analysis uses no new Bazel invocation, profile processing or action logging.
Its parsing/aggregation duration is recorded as `analysis_seconds`; report
serialization and the existing artifact upload add a small unmeasured overhead.
Malformed files produce an unavailable entry without raw exception details.
The independent, always-run analysis step is non-blocking even if the build or
profile summary fails. Existing profile summaries and dashboard data are unchanged.

The VS job keeps Bazel's prepared external repositories on the runner under
`vs-cache/repo-contents`. Master pushes, same-repository PRs and manual runs
each use a separate directory. The builder mounts the selected directory at
`/bazel_repo_contents_cache`. Bazel 8.5.1 can reuse repositories that declare
reproducible contents across fresh output bases, including the SymCrypt LLVM
distribution. The job still clears its download and action caches before building.

The existing `cleanup_runner_bazel_cache.py` entry point accepts
`--retain-repo-contents` to apply a shared prepared-repository retention policy
after its ordinary selected-cache cleanup. The native VS workflow opts in
before disk preflight and builder startup; other callers remain unchanged.
The policy bounds only prepared repositories across all three scopes. Defaults are a
**70 GiB combined budget**, **14 days since last use**, and **220 GiB free on the
cache/workspace filesystem**. Repository Actions variables
`VS_REPO_CONTENTS_CACHE_MAX_GIB`, `VS_REPO_CONTENTS_CACHE_MAX_AGE_DAYS`, and
`VS_REPO_CONTENTS_CACHE_MIN_FREE_GIB` override these positive integer limits.
The initial limits are provisional; measure peak cold/warm build disk usage
before treating them as a capacity guarantee. Existing Docker, download,
action-cache and package-cache cleanup and the separate Docker/workspace
preflight remain unchanged.

The shared `retain_repo_contents_cache.py` policy understands Bazel **8.5.1**'s
prepared-cache layout. Last use comes
from the mtime of `<N>.recorded_inputs`, which Bazel refreshes on cache hits,
not from repository directory mtime. It acquires nonblocking POSIX exclusive
locks on every existing scope's `gc_lock`, compatible with Bazel's own locks,
and refuses redirected paths, unknown layouts, mounts and Docker references.
Run it only on an idle, dedicated agent, never against an active developer
cache. It removes expired entries, then evicts oldest-unused entries until
both budget and free-space limits are met. Interrupted eviction trash and
unusable orphan entries are reclaimed on the next pass. Atomic moves to
`_trash` precede guarded deletion; cache-root, scope and lock files survive.
Usage/free-space checks are repeated after eviction batches to account for
hardlinks and actual reclaimed space. If deleting eligible entries cannot
provide headroom, the step fails rather than starting the build.

Preview on an idle runner with its actual workspace path:

```bash
python3 -B -m tools.bazel.ci.retain_repo_contents_cache \
  --runner-workspace /home/azureuser/actions-runner/_work/sonic-buildimage \
  --max-gib 70 --max-age-days 14 --min-free-gib 220 --dry-run
```

Dry-run logs planned evictions and estimated remaining usage/free space; it
does not delete contents, but may create missing `gc_lock` files. Normal runs
log measured totals per scope and add a job summary. A separate
`--check-free-space-only` invocation in host preflight checks the 220 GiB
headroom without scanning, locking or modifying cache contents.

The future opt-in post-job supervisor in #286 can reuse the same cleanup API:

```python
cleanup_caches(
    runner_temp, runner_workspace, remove_package_cache=True,
    repo_contents_policy=dict(
        max_bytes=70 * 1024**3, max_age_days=14,
        min_free_bytes=220 * 1024**3))
```

After its existing final-result, publication, storage and idle-host guards,
the supervisor can perform retention before admitting the next job. Newly
created GC locks retain the scope directory owner's UID/GID even when invoked
by a root supervisor; existing ownership and permissions are never changed.
This change does **not** modify #286, deploy a service, or enable deferred cleanup.
Until that integration is reviewed and deployed, ordinary VS jobs keep the
pre-build cleanup fallback. When eviction moves post-job, keep the cheap
free-space-only check in the workflow and ensure fallback remains for unenrolled
jobs. This is not a hard during-build cap: an active build can grow the cache, and
other users of the filesystem can consume space. Bazel's own idle age GC is
still enabled, but the host pass does not depend on idle GC completing before
the ephemeral builder exits. Nothing changes for direct developer builds.

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
The downstream installer verification job extracts the release and commit from the packaged filesystem.

The host wrapper [`build_vs.sh`](build_vs.sh) runs one
`make target/sonic-vs.bin` request
with the native AMD64 Trixie VS configuration, Bazel, FIPS and DASH enabled.
The **Build sonic-vs.bin through the host Make wrapper** step ends when Make
finishes producing the ONIE installer and its selected container archives.
It does not request those archives again or run the post-build validators.
This target does not require KVM. BIOS/UEFI VM disk generation and guest boot
checks belong to `target/sonic-vs.img.gz` and are not exercised by this `.bin`
CI path. The separate VM target retains those checks.

Native VS CI enables `SONIC_VS_RFS_HANDOFF=1` for a job-local handoff of a
freshly built stage-one rootfs. The wrapper creates private staging for each
invocation and cleans it after Make succeeds or fails. After stage-one
compression and normal cache publication succeed, the native AMD64 VS ONIE
recipe moves the unmounted tree into staging. Final assembly verifies the
archive path, stat and SHA256 and the tree identity, then consumes it once
instead of deleting it and running `unsquashfs`. Cached or already-existing
archives, missing or invalid staging, other images and cross builds retain
ordinary extraction. The intermediate SquashFS and its cache publication
are deliberately retained; this optimization does not skip its compression.
The offline image bundle, Docker first-start import and installed-image
validators retain their formats and integrity checks.

When the host-base cache is enabled, staging also requires its current
`artifacts/vs/host-base-cache.json` receipt to report a native cache miss and
match the SquashFS digest and size. Cache hits, missing or invalid receipts,
and mismatched archives use ordinary extraction, even if an older rootfs tree
remains in the checkout.

Unset `SONIC_VS_RFS_HANDOFF` to disable the handoff. The wrapper reports
`restored`, `miss`, `stored` or `unused`, plus available store/restore timings.
Require `restored` before attributing an extraction saving to this feature.
Archive hashing is real overhead: compare total build time and final assembly
on comparable runners with identical inputs, rather than treating a skipped
extraction as a measured end-to-end speedup.

Native VS CI also enables `SONIC_VS_FAST_ASSEMBLY=1`. In `build_vs.sh` this
selects `SONIC_BOOT_LAYER_GZIP_LEVEL=1` for layers that need gzip normalization
and `SONIC_SQUASHFS_ZSTD_LEVEL=3` for the final rootfs. Existing gzip layers
are verified and reused without recompression. Compression levels are
declared Bazel action inputs; changing the profile also invalidates an
existing VS installer through its Make stamp. Explicit level overrides take
precedence. Ordinary builds retain gzip level 6 and the existing SquashFS
default; unset the fast flag and both overrides to use those defaults.
The faster profile trades installer size for compression time, not a new
compression format. Compare size, installation, first boot and total build
time on matched runners before claiming an end-to-end improvement.

The fast profile also defaults `SONIC_VS_BATCH_PACKAGES=1`. Native AMD64 VS
extensions combine only adjacent APT pairs (`efitools`/`mokutil` and
`python3-dev`/`j2cli`) and two contiguous pip blocks. Existing version constraints,
dependency resolution, proxy handling and cleanup hooks remain unchanged.
Other platforms, architectures and cross builds retain serial installation.
Set `SONIC_VS_BATCH_PACKAGES=0` to disable batching independently of compression,
or set it to `1` without the fast profile to test batching alone. Changing this
setting invalidates the VS installer Make stamp, not container-layer actions.
Repeated fresh-root lab tests with frozen inputs saved about 13 seconds in
these blocks and preserved package state and Python payloads; downloads were
excluded and this is not a full-image speedup claim. Batching does not change
the offline-bundle format, image tags, first-start Docker import or ONIE wrapper.

Archive publication and fallback member projection attempt Linux reflinks,
then copy normally when the filesystem does not support them. The destination
has an independent inode, so later writes cannot corrupt a Bazel/cache source.
Publication still uses a temporary file and atomic replacement.

With `SONIC_VS_IMAGE_TIMINGS=1`, the wrapper also preserves fixed-label
command durations under `artifacts/vs/image-timings/installer-steps`.
Receipts cover restoration/extraction, the extension, initramfs, version
collection, package cleanup, size scans/optimization and SquashFS. They do
not capture command arguments or output, and recording failure does not
change a completed command's status. Extension time includes offline-bundle
work; these nested measurements must not be added to the existing phase
totals. Boot-bundle receipts additionally report native/archive selection,
layer action counts, compression level, graph setup and total wall time.

The build publishes the installer, its Make image list and one Actions artifact
per selected OCI family. Each family includes the OCI configuration, ordered
layer and archive digests, and the inputs needed by its installed probe. It
also publishes the base and shared SWSS layer for their own runtime checks.
Each consumer verifies the producer revision and the release extracted from
the installer. Common checks cover the exported archive, loaded platform,
configuration and filesystem layers. Container owners retain their service
checks, such as a real Teamd control transaction or a Config Engine YANG render.

To exercise runtime containers, dispatch **Bazel OCI** with `skip_vs=false`.
Set `enable_debug_layer=true` to publish and validate registered debug
companions. Pull requests and master pushes use the runtime-only setting.
`sonic-vs-bazel-timings-amd64` records the Make/Bazel build. The per-container
job logs and `validated-*` artifacts report validation separately.
ARM64 source/package tests run independently. The VS installer publishes
AMD64 service images.

For local investigation, the individual build and rebuild tools remain available.

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
out of public artifacts. The VS timing artifact may additionally publish the
sanitized BEP metric/result events described above; raw events stay private.
A failed job's container-evidence selection publishes only a validated partial summary.
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

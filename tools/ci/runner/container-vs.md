# Build the VS installer in a SONiC slave runner

## Dedicated host-agent route

The active `vs_container` workflow job now selects
`[self-hosted, linux, bazel-x64, vs]`. The Actions runner stays on the host;
the normal Make wrapper creates the SONiC build containers. It does not need
the external worker manager, image marker, or the container-runner UID/layout
described in the legacy profile below.

Provision one non-root runner per dedicated AMD64 VM with Docker/buildx access,
Git, Make, Python 3, wget, `j2`, a readable system CA bundle, and host legacy
IPv4 NAT support (`iptable_nat`). The runner's file-creation mask must be 0022.
Allow the nested Docker and Bazel sandbox capabilities required by the existing
Make builder, including applicable host AppArmor rules; do not globally disable
host protections. The kernel sandbox is exercised in the Trixie builder before
the installer starts.

### Slave image publication and selection

The [SONiC slave images workflow](../../../.github/workflows/sonic-slave-images.yml)
runs on master pushes or manual dispatches on master. It builds or reuses the
AMD64 Bookworm and Trixie slaves and pushes their computed tags to
`sonicdev-microsoft.azurecr.io`. It does not create Git commits or publish
mutable `latest` tags. Publication and installer jobs may run independently
on available dedicated host agents; there is no shared job-level queue.

Before either the publication or VS installer job's disk-space preflight, it runs
`docker image prune --all --force` to remove all unused Docker images, including
tagged slave and base images, regardless of age. Images referenced by running or
stopped containers are retained. The job also runs
`docker builder prune --all --force` to remove all eligible unused build cache
regardless of age, but does not prune containers or volumes. Run this only on a
dedicated agent with no other jobs sharing its Docker daemon; future builds may
need to pull or rebuild removed images. Preflight still requires sufficient free
space after cleanup. After each successful publisher push, it removes only that image's
local computed tag and registry-qualified tag, without forcing removal.
Failed pushes retain their local images. Docker manages reclamation in its
containerd image store; never delete containerd storage files directly.
Shared layers and retained build cache may continue to occupy disk space.

Both jobs also remove only `_github_home/.cache/bazel`, `bazel-disk`, and
`bazel-repo` under `RUNNER_TEMP`, plus `vs-cache/bazel` under the runner workspace,
before the disk-space check. This reclaims local Bazel state, disk-cache entries,
and repository downloads left by previous container and VS builds. The remote
cache and workflow cache settings are unchanged; missing local data must be
fetched or rebuilt. The VS job recreates its Bazel directory and prepares source
distdirs after preflight. Cleanup refuses redirected paths,
remaining mounts, or references from running or stopped Docker containers, and
reports failures rather than continuing to preflight. Use an idle dedicated agent
with no active build processes using these directories. The VS installer also
removes the obsolete local `vs-cache/packages` directory, but only after its NFS
package cache passes validation. Publisher cleanup leaves local packages alone.
Neither cleanup removes `_temp`, other home/cache directories, NFS caches, or checkouts.

For pre-merge testing, pushes to the same repository's `vs-runner` branch
temporarily also publish real images to that registry. Manual dispatch remains
master-only, and pull requests do not trigger publication. Remove the `vs-runner`
push trigger and job-condition allowance before merging.

The publisher uses the Azure slave pipeline's `PLATFORM=generic` profile for
AMD64, with FIPS enabled and `SONIC_VERSION_CONTROL_COMPONENTS=py2,py3,web,git,docker`.
Both workflows explicitly enable `MIRROR_SNAPSHOT=y`, selecting timestamped Debian
repositories under `http://packages.trafficmanager.net/snapshot` instead of the
ordinary Debian mirror. This intentionally differs from the Azure profile's
default mirror mode and changes the computed slave tags. The installer remains
`PLATFORM=vs` with the same snapshot policy for slave and runtime image builds.
The base-image registry, Docker build command and APT retry policy retain their
repository defaults. Make's existing slave selection and build machinery are
used without slave-specific Makefile extensions.

Make computes the image tag from the current checkout's generated slave inputs;
there are no fixed slave tag overrides. Keep the image-affecting settings aligned
when changing either workflow. Matching the Azure build profile does not guarantee
matching tags across different source revisions or build-hook versions.

The installer enables `ENABLE_DOCKER_BASE_PULL=y`, retaining Make's normal order:

1. Reuse a local slave image with the computed tag.
2. Otherwise pull that tag from the registry and tag it locally.
3. If the pull fails, log the failure and build the slave locally.

This includes registry/network pull failures, not only missing tags. A failed
local build still fails the job. The publisher and installer may queue in either
order after a master push; publication is an optimization, not a prerequisite.
An installer can build locally if the publisher has not supplied its tag yet.
Shared package-cache writes remain enabled with the computed build environment.

Both jobs install Azure CLI if missing and authenticate using
`az login --identity`, explicitly select the ACR subscription, then obtain an ACR
token with `az acr login --name sonicdev --expose-token`. The helper passes the
token through `docker login --password-stdin` using the exact
`REGISTRY_SERVER:REGISTRY_PORT` endpoint used by Make and publication, currently
`sonicdev-microsoft.azurecr.io:443`. This avoids storing credentials only for the
hostname without the port and leaves Make's registry formatting unchanged.
The token is not printed or passed as a command-line argument.
Set the repository Actions variable `ACR_SUBSCRIPTION_ID` to that subscription's
ID or name; both workflows pass it as `AZURE_SUBSCRIPTION_ID`. The helper fails
before authentication if it is missing, and stops if subscription selection fails.
No subscription ID is stored in source code or inferred from the agent's default.
Both workflows set `ACR_REGISTRY_NAME=sonicdev` separately from the Docker
login server `REGISTRY_SERVER=sonicdev-microsoft.azurecr.io`; the Azure resource
name must not be derived from this hostname.
The host needs a system-assigned managed identity with registry pull permission;
the publication job additionally needs push permission (`AcrPush` for this
registry's `LegacyRegistryPermissions` mode). Successful login alone does not
prove push permission. Installing Azure CLI
requires noninteractive sudo on Debian/Ubuntu hosts. Azure CLI state is stored
in the job's runner temporary directory. Each job also creates a private Docker
configuration directory there, named with the run ID, attempt and job ID, with
mode `0700`. An initialization step exports `DOCKER_CONFIG` through `GITHUB_ENV`
before host Docker commands, so login, Make's host-side slave pulls and publication
share the private configuration rather than `~/.docker/config.json`. Registry
credentials are not mounted into the slave containers.
An independent `always()` step logs out of the exact registry endpoint and removes
the private `config.json`, even if logout fails; logout failures still fail the
job. It runs from the runner temporary directory, independently of checkout
availability and artifact-upload results. An abrupt agent/VM termination can
prevent this step from executing. Registry login failures stop the job before
Make, rather than silently bypassing authentication.

Run `python3 -B tools/ci/runner/preflight.py --host-agent --workspace "$PWD"
--build-target sonic-vs.bin` from the checkout as the runner user. This retains
the CPU architecture, process umask, Ghostscript policy, legacy NAT, workspace
mount, Docker/buildx, memory, disk-budget and template checks without requiring
the legacy `sonic-vs-runner.service` or a `/data` installation. At least 10 GiB
memory must be available; allow headroom beyond that threshold. The workspace
and Docker store each need 100 GiB free, or 200 GiB if on the same filesystem.
These are admission limits, not guaranteed maximum build sizes.

Each run uses a distinct checkout and label-scoped builder cleanup. Local Bazel
state lives in `vs-cache/bazel` under the runner workspace and is cleared before
preflight. The VS installer uses `/nfs/dpkg_cache/vs` for Make's package/image
cache, mounted into the slave as `/dpkg_cache`. Only push builds on `master` use
`rwcache` (read and publish entries). All other builds, including PRs and manual
runs, use `rcache` (read entries only).
Matching dependency/configuration keys are required for hits. This does not
replace Bazel's remote cache or resolve its connection failures.

The VS job sets `SONIC_BUILD_JOBS=4` for concurrent Make recipes. Per-package
compilation parallelism remains at its existing default, and Bazel retains its
separate `--jobs=4` limit. Commands sharing one Bazel output base still serialize;
increasing Make concurrency does not bypass that lock and may increase memory
or I/O pressure.

Before local cache cleanup, the job uses noninteractive `sudo` to check that the
cache directory exists and is NFS-backed. The `stat --file-system` probe triggers
on-demand mounts and checks the filesystem actually reached, rather than an
`autofs` mount-table entry. An unresolved `autofs` filesystem or a local
filesystem is still rejected. These privileged host checks match
source-path resolution by the rootful Docker daemon: the non-root runner does
not need to traverse `/nfs` itself. Read/write access and existing permission
initialization remain inside the slave through the unchanged `Makefile.cache`.
There is no local fallback, directory creation, or permission repair for the
NFS cache in the workflow. Provision access for the Docker daemon to mount the
source and for the slave build user's UID/GID to use `/dpkg_cache`, accounting
for root squashing.

After artifact upload attempts, the job removes its labeled containers and then
its own `vs-<run-id>-<attempt>` checkout, including per-run nested-Docker state.
Checkout deletion requires noninteractive `sudo rm` for root-owned build output.
It refuses symlinked checkouts, remaining host mounts, and references from any
Docker container. If container cleanup or an artifact upload fails or is
interrupted, checkout deletion is skipped so the remaining evidence is retained.
An installer upload skipped because the build failed does not block cleanup
after the always-run evidence uploads succeed.
Missing cleanup tooling in a partial checkout also fails explicitly; inspect
and clean that checkout manually after verifying it is unused.
End-of-job cleanup does not prune the host Docker daemon, other jobs'
checkouts/containers, persistent caches, or reusable images/build layers.
Startup image/build-cache pruning is described above. Operators must still manage
cache/image retention and leftovers from interrupted runners; cleanup cannot
run after VM termination or a runner crash. This does not move Docker's data
root or remove the free-space admission checks.

The job explicitly passes `grpc://172.16.0.4:9092` as the Bazel action cache and
remote downloader through `BAZEL_CONTAINER_ARGS`; verify connectivity inside
the Make builder as well as on the host. PR/manual builds remain read-only,
while master pushes enable uploads. The legacy HTTPS authentication override
is disabled for this job so it cannot replace the gRPC cache while leaving the
gRPC-only downloader enabled. Repository-wide defaults remain unchanged.
The isolated kernel build separately receives
`SONIC_BAZEL_KERNEL_REMOTE_CACHE` and `SONIC_BAZEL_KERNEL_REMOTE_UPLOAD` through
the Make wrapper; it does not inherit `BAZEL_CONTAINER_ARGS`. Its upload policy
also permits writes only for master pushes. See the
[kernel cache interface](../../bazel/kernel/README.md#remote-cache-through-make).

The host job has no global installer queue. Assign the `vs` label only to
independently provisioned, adequately sized VMs with one active runner each.
Workflow-level cancellation rules still apply. Run a full installer and its
image-validation stages successfully before considering the migration verified.

## Legacy container-runner profile

The previous `VS installer in SONiC slave (AMD64)` job ran the
Actions worker inside the Trixie build container. It used the
`sonic-slave-container` runner label. The incoming K3s variant uses
`sonic-slave-container-k3s`; both invoke the native build directly:

```bash
PREFER_DEBIAN_REPO=y make -f slave.mk target/sonic-vs.bin \
  BUILD_WITH_BAZEL_WHEN_AVAILABLE=y \
  INCLUDE_VS_DASH_SAI=y \
  INCLUDE_FIPS=y \
  ENABLE_ASAN=n \
  ENABLE_SYNCD_RPC=n
```

This produces the ONIE installer. It does not create or boot BIOS/UEFI VM disks.
The registered Bazel container pairs also produce matching debug archives;
the normal installer continues to select the runtime archives. Packages without
Bazel support retain their existing Make producers.
CI explicitly sets `PREFER_DEBIAN_REPO=y` for Monit and rasdaemon to fetch their
pinned Debian source archives and packaging with SHA256 verification. The
default `n` preserves the existing Git source recipes. Both paths keep the
SONiC patches, Debian quilt patches, and runtime/debug package outputs.
Libyang3 retains its upstream test harness; its headless test and error-reporting
improvements are tracked separately in
[PR #83](https://github.com/securely1g/sonic-buildimage/pull/83).
DHCP relay retains the upstream submodule revision and existing test behavior.
Its test recipe suppresses failures, so installer success does not establish
that the DHCP6 tests passed. The isolated Redis fixture and failure-reporting
fixes remain a separate component change in
[sonic-dhcp-relay #1](https://github.com/securely1g/sonic-dhcp-relay/pull/1).

The native P4 dependencies still use their existing Make/Debian producers.
The GitHub-hosted Ubuntu OCI regression job stages the nine source files pinned in
[`src/p4lang/source-inputs.json`](../../../src/p4lang/source-inputs.json).
It uploads a per-run artifact; the native job selects its exact artifact ID
and independently checks every file's SHA256, size and descriptor identity.
It then passes `P4LANG_SOURCE_BASE_URL=file:/sonic/artifacts/vs/p4-source-inputs/sources`
to Make. Staging is separate from `src/p4lang`, where `dget` replaces its working
descriptor. Ordinary Make invocations keep the original OpenSUSE URL.

OpenSUSE currently publishes an inconsistent P4C descriptor: its unchanged
bytes identify an orig archive with SHA256 `61251b9c...` and size 173,765,465,
while both its mirror and direct content server supply `a41d616a...` and
173,758,549 bytes. The supplied archive matches SONiC's existing `versions-web`
record and the prior passing native build. The input manifest binds this exact
known discrepancy to the descriptor and archive hashes; receipts report it
explicitly, and any new discrepancy fails verification. This does not establish
equivalence with the archive named by the descriptor. The original descriptor
and existing `dget -u` behavior are preserved; no checksum rows are rewritten.
PI and BMv2 must match their descriptors normally.

This source handoff uses a fresh CI artifact. It adds no persistent P4 source
cache and changes no shared Make-cache logic. The tracked source pins are
already covered by the P4 recipes' existing input-file hashing.

The [K3s runner controller](https://github.com/securely1g/agi-projects/tree/main/sonic-slave-wrk-mgr/k3s)
registers a replacement worker after each job. Preparation verifies the manager
revision recorded in the runner image against the workflow's `EXPECTED_MANAGER_REVISION`.
The native profile requires a privileged container, private PID/cgroup namespaces,
bridge networking, and passwordless sudo for its UID 1000 build user. Each job
starts its own Docker daemon; the host Docker socket is not mounted. The Actions
work volume is mounted at `/actions`, allowing the checkout to be bind-mounted
at `/sonic` for existing build hooks without hiding the runner's own files.

Preparation runs the pinned kernel Bazel sandbox as UID 1000 and stops if
the worker lacks the required namespace permissions. Node-based Actions
use the image's existing system CA bundle for verified HTTPS connections.
The manager also declares `SONIC_BAZEL_JAVA_TRUST_STORE` in the image. The
isolated kernel adapter reads that default and records the selected trust
store's digest; the Make rule contains no runner-specific certificate path.

`prepare_vs_slave.sh` checks the image identity and native capabilities, prepares
the build hooks and configuration normally supplied by the outer Make wrapper,
and configures VS through `slave.mk`. The workflow supplies `SONIC_BUILD_GIT_NAME`
and `SONIC_BUILD_GIT_EMAIL` for package recipes that create temporary Git commits;
preparation sets this identity only in the ephemeral worker's user configuration.
The VS selector is passed to `configure` and saved in `.platform`; it is not
exported as a runtime hardware identity to native package tests.
It checks that the selected installer has
no Bookworm container prerequisites. A future configuration needing those
prerequisites must add an explicit compatible preparation stage.

The K3s controller on lgh117 maintains four disposable workers on lgh100.
Each has a six-CPU and 20 GiB limit. One full VS job runs at a time across
the pool. Before building, the workflow requires 256 GiB available on the
workspace filesystem. Make still uses the shared `/dpkg_cache` in `rwcache`
mode; that cache holds Debian packages.

Bazel uses only the worker-configured remote cache. Its disk and repository
caches are disabled. The short output base, Bazelisk downloads, and verified
source archives are private to the pod and disappear when it is replaced.

The shared APT policy allows eight retries per transient acquisition failure
(nine attempts), with exponential delays capped at 30 seconds and 20-second
HTTP/HTTPS/FTP timeouts. Each index or package download has its own retry
budget, including dependencies fetched while repairing a local-DEB install.
APT retains its normal error classification and TLS, signature and hash checks.
Index refreshes fail if any index cannot be downloaded, so an incomplete
refresh cannot become a successful cached Docker layer. These settings bound
individual attempts and delays, not the total duration of an APT command.

Legacy native Docker profiles can set `SONIC_DOCKER_APT_RETRIES=0` to avoid APT 3.0.3's
unbounded delayed-retry warning queue ([Debian bug 1117797](https://bugs.debian.org/1117797)).
The Dockerfile generator installs a temporary APT fragment with a late-sorting
name and verifies that APT's effective retry count is zero before each independent
stage's hooks and first build command, including builder and runtime stages.
It refuses a pre-existing override path and removes only its
own unchanged fragment before the stage can be copied, flattened or exported.
The resulting image keeps the original eight-retry runtime policy. Unset or
empty leaves ordinary Make behavior unchanged; other values are rejected.
The active host-agent workflow leaves this setting unset.

Legacy native Docker profiles can use the existing `run_with_retry` helper
with two retries and a ten-second delay. A failed Docker build still fails
after three attempts, and later attempts can reuse completed layers. The active
host-agent workflow uses Make's default Docker build command instead. Strict
index refresh errors, timeouts, and TLS, signature and hash checks stay enabled.
This CI-only mitigation does not change rootfs or native-package APT behavior,
repair a persistent transport failure, or force APT to refresh DNS.

The worker supplies the endpoint and scoped credential helper. CI verifies
that remote reads and writes are enabled and probes access before Make.
If that check fails, the job stops; there is no local-cache fallback.

The former `Make VS with Bazel SWSS (AMD64)` job is disabled in this workflow.
After the new native build, CI verifies the installer container
selection and both runtime/debug archive pairs, runs the P4/debug checks in the
same slave, and publishes `sonic-vs-slave-amd64` with the installer, matching
container archives, and SHA256 checksums. `sonic-vs-slave-evidence-amd64` retains
the configuration and validation receipts.

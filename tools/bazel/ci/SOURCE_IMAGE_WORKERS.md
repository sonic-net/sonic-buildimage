# Dedicated source-image workers

The `Bazel OCI` workflow's `source_image=preflight` dispatch measures all four
`sonic-slave-container-dpool` workers. Each matrix entry also requires the
worker's unique name label, so four successful jobs identify four distinct
workers. Ordinary pull-request and default dispatch behavior is unchanged.

The preflight records the immutable slave image, CPU quota, memory limit, free
workspace/cache space, required tools and private namespace checks. It requires
native AMD64 Debian Trixie, UID/GID 1000, 20 GiB or more of bounded memory,
2–6 CPUs, and at least 50 GiB free on each persistent workspace/cache mount.
The two free-space measurements may refer to the same underlying disk; they
are not additive capacity. It does not run Make, Bazel builds, Docker image
assembly, or Debian packaging. A pass establishes this resource profile only;
actual image builds and loaded-container acceptance remain separate gates.

Example, using the reviewed workflow branch:

```sh
gh workflow run bazel-oci.yml --repo securely1g/sonic-buildimage \
  --ref codex/source-image-worker-dispatch \
  -f source_image=preflight -f skip_vs=true
```

Each job uploads its measured `preflight.json`, including failures. The job log
prints the same safe receipt; no environment dump or host credentials are
included. Affinity labels are managed by the runner operator, not this job.

The initial [four-worker measurement](https://github.com/securely1g/sonic-buildimage/actions/runs/41693647811)
found 4 CPUs and 10 GiB per worker. All isolation and tool checks passed, but
that allocation did not meet the original 16 GiB requirement. The later
[Sysmgr worker run](https://github.com/securely1g/sonic-buildimage/actions/runs/41706960327)
built both images with a measured 20 GiB limit and a 14.69 GiB peak, without
OOM events. Its [cached retry](https://github.com/securely1g/sonic-buildimage/actions/runs/41717422930)
also passed static, incremental and loaded validation after adding the missing
archive tool. The [HA worker run](https://github.com/securely1g/sonic-buildimage/actions/runs/41706956620)
also passed build, static, incremental and loaded checks, with a 20 GiB limit
and a recorded peak of 21,475,430,400 bytes (approximately 20 GiB), without
OOM events. It had almost no recorded memory margin. The preflight requires
the tested 20 GiB minimum; 24 GiB per worker is recommended for HA and other
large image graphs when host capacity permits. Four simultaneous 24 GiB
workers require at least 96 GiB for their limits, plus host/daemon headroom;
separate containers do not create additional host capacity. The original
10 GiB measurement does not establish enough build capacity.
The preflight selects scheduling from the actual cgroup limits. Admitted workers
with at least 4 CPUs use two Bazel actions, two scheduled CPUs and 8000 MiB
scheduled action memory. Other admitted workers (at least 2 CPUs) retain one
action, one scheduled CPU and 4000 MiB. Both profiles keep the 20 GiB admission
minimum, 3 GiB Java heap and 4 GiB loaded-probe limit. No profile admits the
historical 10 GiB workers.

These options follow the selected driver's arguments so they override its
defaults. They change scheduling, not compiler features. `worker-preflight.json`,
`worker.json` and each phase receipt record the selected profile, its minimum
limits and the actual measured limits. Selection is fixed during preparation;
a later phase rejects changed limits instead of silently changing its launcher.
The two-action profile remains a pilot until actual graph and image acceptance
establish its behavior. The earlier successful image runs used one action and
do not establish the new profile's peak memory or speedup.
The Bazel JVM uses the worker's readable system Java trust store, as the
existing native launchers do; certificate verification remains enabled.

To build an image, choose `source_image=sysmgr`, `dash-ha`, `orchagent`, `sflow`,
`otel`, `syncd`, `gearbox`, `dash-engine`, `gnmi`, `gnmi-no-dialout` or `teamd`, a `source_worker` affinity label and a full `source_revision` SHA.
For example, the accepted HA source can be tested on worker 1 with:

```sh
gh workflow run bazel-oci.yml --repo securely1g/sonic-buildimage \
  --ref codex/source-image-worker-dispatch \
  -f source_image=dash-ha -f source_worker=sonic-slave-lgh114-1 \
  -f source_revision=e9178c15401018fa7b6bf944574cdad678660c87 -f skip_vs=true
```

The dispatcher checks out its own reviewed code separately from that exact
source revision. Fixed driver records reuse each image's existing build,
static validation, incremental edit/restore and loaded-container checks.
The disposable slave installs only missing archive/debug execution packages
(`libarchive-tools`, GDB and binutils), checks their tools before building and
retains the installed package versions. These do not enter the image layers.
Sysmgr, OTEL, Syncd, Gearbox, DASH engine and both gNMI profiles use `review_bazel.sh` with each workflow's immutable catalog;
the other four use their compatibility entrypoint and verify any workflow catalog
agrees. No module or source-owner override is added. Selected Rust modules use
`rules_rs` with tracked Cargo inputs; their normal module extension prepares
crate repositories. Sysmgr initializes its existing gNOI submodule. The image
drivers retain their metadata-only `prepare_manifests.mk` step. OTEL reuses its
existing runtime/debug image driver and installed collector probe, including
private Redis and Supervisor checks. Its hosted producer-contract tests remain
a separate required validation; the worker does not replace them. Syncd and
Gearbox reuse their existing source image drivers and installed service probes.
Syncd uses the declared regular VS startup fixture; this is not a full VS or
DASH forwarding run. Before its image build, the worker verifies the standard
`/dev/net/tun` character device and creates then closes a nonpersistent TAP
in a disposable network namespace. The retained `worker-tap.json` proves this
capability without creating a VM or using KVM. An already active Gearbox acceptance run must finish
before another worker run for that image is considered.

DASH engine reuses its `dash_engine_image.py` and `dash_engine_loaded.py`
drivers. Its loaded check creates disposable `eth1`/`eth2` interfaces inside
the private image container and runs the original startup script and installed
BMv2 Python clients. It requires no host TAP device, VM or KVM. The dispatcher
entry makes this acceptance runnable; it does not establish a passing image
build or replace the separate owner tests for PI, BMv2 and nnpy.

The hosted source-download job reuses the existing VS handoff for the five
GitLab-hosted `rules_gzip` dependencies. It fetches repositories only, then
checks the BCR SHA512 values. The native worker verifies those bytes again
with the same `source_archive.py` before adding its persistent distdirs.
This avoids requiring the native worker to reach GitLab and changes no
source archive, module version or checksum. The download receipt is retained.
An optional numeric `source_inputs_run` can reuse this repository's existing
`bazel-repository-inputs` artifact instead of waiting for another hosted fetch.
The dispatcher requires the successful original source-download job, matching
repository/run/source provenance and one nonexpired artifact. It checks the
complete expected file inventory and original receipt before the fixed hashes.
The legacy artifact's Redis test-helper files are recognized but never used.
The selected run/job/artifact identities are retained with the worker evidence.

Workers on the same manager share downloaded repositories and action results
through `SONIC_BAZEL_CACHE_ROOT` (default `/bazel_cache`), using its `repository`,
`disk`, `bazelisk` and `distdir` directories. An existing `BAZELISK_HOME` under
the shared cache mount is respected. Runner names do not partition these
caches. The five source archives are still verified for every handoff and
published atomically; downloaded artifact inventories and receipts stay in
the worker's private temporary directory.

Each worker keeps its Bazel server and output tree under
`SONIC_BAZEL_STATE_ROOT/<image>/output-base` (default
`/actions/bazel/<image>/output-base`). The manager must mount a different
`/actions` work volume into each worker; equal container-visible paths then
refer to distinct output trees. Cache paths must remain under `/bazel_cache`,
state paths under `/actions`, without symlinks, traversal or overlapping cache
directories. These environment variables configure this dispatcher; Bazel
itself receives explicit repository/disk-cache and output-base options.
Existing per-worker caches are left untouched; this change neither moves nor
deletes them. The job starts its own private
Docker daemon and uses the existing nested-cgroup helper. It shuts down its
idle Bazel server before the loaded-container phase, whose existing 4 GiB limit
is preserved. Memory/peak/anonymous-page/file-cache/OOM measurements accompany
every phase and periodic build-log samples. An OOM event fails the pilot.
Only the stopped job's private Docker data is removed; Bazel caches are kept.

The upload contains declared image/source outputs with verified sizes/hashes,
safe validation receipts and dependency/BEP summaries. Private environment
state, raw invocation records and daemon logs are excluded. This workflow
does not run unrelated owner test suites, package Make recipes, or the full VS
installer. The selected image drivers still audit their configured graph for
DEB generation before building.

The worker also uses Bazel's official mirror for the exact BCR `gawk@5.3.2`
source archive. Its existing SHA256 and upstream build overlay stay unchanged.
Only the three selected GNU/kernel source URLs are rewritten; the original
checksum-verified shared downloader configuration is retained. The worker and
hosted source-base/archive jobs select the same registry policy through
`source_mirrors.py`, including the release mirror for the unchanged BCR pigz
archive. Phase resource
receipts include the observed cgroup memory/swap limits as well as usage and OOM
counters, so a runtime limit change is not confused with a successful capacity test.

For four observed Debian snapshot HTTP 503 failures, the downloader retains the
original timestamped URL and adds its verified official content-addressed
`snapshot.debian.org/file/` endpoint. Both endpoints returned identical bytes
matching the checked-in SHA256 and size. The URL's SHA1 identifier only locates
the file; Bazel continues enforcing the original SHA256. Other snapshots,
package versions and architectures are unchanged. Public Syncd/Gearbox policy
validation recognizes only the exact checked-in `DEBIAN_FRONTEND=noninteractive`
constant and retains the policy's original bytes and hash.

The two fixed gNMI selectors use the existing `gnmi_image.py` driver:
`gnmi` always passes `--dialout enabled`, and `gnmi-no-dialout` always passes
`--dialout disabled`. Each job retains its own artifact directory and complete
build/static/incremental/loaded receipts. Static and incremental checks read the
built profile from that receipt; the unchanged `gnmi_loaded.py` does the same.
Public publication rejects a receipt whose dialout setting differs from the
selected fixed profile. Both profiles are required for changed-input gNMI
acceptance; a pass for one does not establish the other. They can use separate
idle workers with the same immutable source/catalog. Neither selector adds a
capability, command input, source override or new executor path.

Teamd uses its existing `teamd_image.py` four-phase interface and
`libteam_draft_bazel.sh` catalog wrapper. Before checking out the selected source
or downloading its input artifact, the worker reads `/sys/module/team` and
`/sys/module/team_mode_loadbalance`. Both must already be directories. The same
check runs again at direct preparation entry, before tools, cache preparation
or Bazel. `worker-teamd-kernel.json` retains success or the exact missing module
list; the worker never runs `modprobe` or changes the host. The separate hosted
source-download producer retains its existing ordering and only fetches pinned
source archives. The installed Teamd probe keeps its original private Redis,
network capabilities, control, SWSS and GDB checks. This selector does not replace
owner tests or establish image acceptance.

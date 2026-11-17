# Native AMD64 Teamd images

Make passes `SONIC_IMAGE_VERSION` to the OCI `Tag` label on both archives. The
archive tags are `:latest`. Validation also checks the Common and Rust
supervisor utility version labels inherited from config-engine.

`//dockers/docker-teamd:docker-teamd` and `:docker-teamd-dbg` compose the release
and debug images from the shared source-built SWSS parent. Their `.gz` targets
preserve the existing `docker-teamd:latest` and `docker-teamd-dbg:latest` archive
handoff. The debug image extends the exact release layers.

The only new project payload is the existing libteam owner's
`@libteam//:libteam_utils_runtime`: `teamd`, `teamnl`, `teamdctl`, their manual
pages, configuration and documentation. The same owner's patched `libteam5`
and `libteamdctl0`, plus SWSS's `teammgrd`, `teamsyncd` and `tlm_teamd`, already
come from `//dockers/docker-swss-layer-trixie:docker-swss-layer-trixie`.
The image never compiles a second copy of these libraries.

The utility ELF files require `libdaemon0`, `libjansson4`, `libdbus-1-3` and
`libzmq5` in addition to their source libraries and libc. The shared APT selector
checks these four roots against the actual parent and retains its matching
packages. The runtime check rejects an unexpected additional external package.
All project runtime and matching symbol TARs retain their source owners.

The checked libteam package metadata records the three actual owner outputs,
including the inherited libraries. Package provenance uses the registered
libteam module, matching the shared SWSS parent on maintained `master`. The image adds no runtime account. The
debug image adds the inherited debugger/editor tools, the source FIPS OpenSSH
client and its `_ssh` group contract, and symbols matching the complete native
source chain. Its Vim command follows the full debug editor alternative,
replacing the maintained base image's `vim.tiny` hardlink. Every installed package-state file is checked against its
generated input; no maintainer script or package installation runs in assembly.
Validation preserves every inherited group row in order and the group file's
type, numeric ownership and mode. The debug image may add only `_ssh:x:104:`;
duplicate group names and any changes to inherited passwd/gshadow state fail.

The Make bridge selects native AMD64, VS, Trixie, FIPS enabled, ASAN disabled,
and the standard `dbg` suffix. Other profiles keep the existing producer.
The original `INCLUDE_TEAMD`, host `teamdctl` wrapper, shutdown ordering,
capabilities, mounts and service metadata remain owned by `rules/docker-teamd.mk`.
The selected Bazel archive edge needs manifests and owner TARs, not DEB files or
Make's package cache.

Build and validation are separate phases:

```sh
python3 tools/bazel/ci/teamd_image.py build --artifacts artifacts/teamd
python3 tools/bazel/ci/teamd_image.py validate --artifacts artifacts/teamd
python3 tools/bazel/ci/teamd_image.py incremental --artifacts artifacts/teamd
```

These commands use the shared CI entry point `tools/bazel/ci/libteam_draft_bazel.sh`
with the canonical registry main, the same catalog `.bazelrc` uses.
The build selects explicit runtime/debug TAR and OCI targets and audits their
action graph for DEB-producing outputs and wrappers. Validation checks source
bytes, modes, numeric ownership, inherited files, Make labels, entrypoints,
tagged archive contents and every source ELF/debug pair. A template edit must
change both images and archives while preserving source TARs and the parent;
restoring it must reproduce the original hashes. Timings describe the actual
same-output-base no-op/edit builds, not an inferred cache-hit count.

The installed-image check uses a private Redis helper and an isolated network
namespace with `NET_ADMIN` and `NET_RAW` added. It renders the shipped supervisor template
against that database, runs the startup script, creates a no-port LACP team,
queries its real Unix socket through `teamdctl`, checks the same kernel mode
through `teamnl`, and verifies `teamsyncd` publishes
a kernel MTU change and `tlm_teamd` reads the live daemon state. The debug variant
also exercises source/line/type lookup for all three libteam tools with an empty
symbol-directory negative control. Kernel team support is required; an absent
driver is a test failure. The fixture never uses the host network or physical
interfaces. Forwarding, the complete supervisor boot and full VS installer
acceptance remain separate checks under issue #130.

## CI

The AMD64 archive job in [Bazel OCI](../../../.github/workflows/bazel-oci.yml)
builds and validates both Teamd images alongside their shared SWSS parent. It
reuses the checkout, execution tools, Bazel setup, maintained source mirrors and
persistent cache. The template edit/restore check and matching debug-symbol
checks remain required. The ARM64 archive leg keeps its existing checks; it does
not build the AMD64-only Teamd profile.

A dependent job in the same workflow downloads `teamd-image-amd64-${github.sha}`, verifies
its tested revision and hashes, and runs the installed runtime/debug fixture
above. Its results are retained as `teamd-runtime-amd64-${github.sha}`. These checks run for
pull requests, pushes to `master`, and manual Bazel OCI runs.

## External inputs

APT repositories import only the infrastructure owner's production lock at
`@sonic_build_infra//apt:apt.lock.json`. The four runtime library roots use
an `apt_inputs` subset of `libteam_deps`; this excludes development packages.
The OCI selector generates its runtime lock view directly from those four roots
and the shared infrastructure lock, using the same rule as the SWSS parent.

Debug tools use the existing `sysmgr_debug_debian` collection, which has the
same reviewed roots as the former Teamd debug collection. The test-only Redis
archive uses `@sysmgr_test_debian//redis-server`; it is mounted as test data
and never enters either production image.

The dedicated refresh workspace is retired. Package updates belong to the
infrastructure owner's `apt/sources.json`, `apt/requests.json` and `apt/lock.py`
workflow. Validate the resulting runtime selection and image when package inputs change.

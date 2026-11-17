# Source-built LLDP image

`//dockers/docker-lldp:docker-lldp` and `:docker-lldp-dbg` assemble native
AMD64/Trixie images on the source-built config-engine parent. Their `.gz`
targets preserve the existing Docker-load archive and Make manifest contracts.
The Make selector applies only to VS, AMD64, Trixie, FIPS, no ASAN and the
standard `dbg` marker; other configurations keep their existing prerequisites.

LLDP comes from `@lldpd//:runtime` and its matching debug output. DBSyncd
comes from `@sonic_dbsyncd//:sonic_dbsyncd_runtime`; Common's Python binding
and sonic-py-common are inherited from config-engine. Their owning targets
provide the reusable payloads; no Debian packages are generated. External target libraries
are selected from the production build-infra APT lock. The runtime layer selects the six external libraries
from `lldpd_deps`; debug tools use the shared `sysmgr_debug_debian` set.

The existing Debian LLDP payload places its systemd unit under `lib/`.
The declared upstream BSD-tar action maps that payload into `usr/lib/` to
preserve the parent's merged-/usr symlink. Bytes, ownership, mode, links and
matching symbols are preserved. `_lldpd` owns the existing setuid `lldpcli`
contract; the debug image reserves `_ssh` group 104 because the source
OpenSSH owner's `ssh-agent` TAR already carries that group and setgid mode.

Build and validate in native AMD64 Debian Trixie:

```sh
python3 tools/bazel/ci/lldp_image.py build --artifacts artifacts/lldp
python3 tools/bazel/ci/lldp_image.py validate --artifacts artifacts/lldp
python3 tools/bazel/ci/lldp_image.py incremental --artifacts artifacts/lldp
```

Local builds and CI use ordinary `bazel` with the canonical registry `main`
configured in `.bazelrc`. The pinned DBSyncd module is
`sonic-dbsyncd@2.0.0-8a3830704d3f6067823f7c10b3b31913965add81`, published
from the merged source commit. Validation saves the resolved module graph
and archive metadata with the build receipt.

Run completed-archive checks on a Docker host:

```sh
python3 tools/bazel/ci/lldp_loaded.py --artifacts artifacts/lldp
```

That check creates only private interfaces and Redis inside isolated
containers. It runs the installed LLDP daemons and DBSyncd CLI, verifies
neighbor add/update/delete in APPL_DB, executes the shipped manager's
hostname control method, and checks real source/symbol pairs in the debug
variant. The private root controller retains the capabilities needed by the
installed privilege-separation contract: network administration/raw packets,
UID/GID changes, chroot, file ownership/access and process cleanup. The
setuid CLI keeps its owner mode, so this probe permits that UID transition.
The root filesystem stays read-only, network access is disabled, and Redis
listens only on the fixture's Unix socket and loopback TCP port. This is not
a full supervisor lifecycle or hardware test. The
incremental check edits a template, requires both image/archive outputs
to change while owners and parent remain unchanged, and restores exact
original hashes. Post-build validation is separate from image production.

Receipts record the tested commit and its immutable parent order. CI binds the
head/base to the checkout before building or accepting downloaded archives.
Both runtime and debug variants and their exact recorded hashes are required.
Generated evidence belongs under `artifacts/<name>`; untracked source files
and tracked source edits cause acceptance to fail.

The image version is passed through the `image_version` build setting to the
Make `Tag` label. Inherited component labels use the installed package receipt,
including Common and the Rust supervisor listener.

The LLDP layer preserves the parent dpkg permission records and adds
`_lldpd adm 4750 /usr/sbin/lldpcli`. Static validation checks this record as
well as the actual installed owner and mode of the client.

# Native AMD64 FRR images

Make passes `SONIC_IMAGE_VERSION` to the OCI `Tag` label on both archives. The
archive tags are `:latest`. Validation also checks the Common and Rust
supervisor utility version labels inherited from config-engine.

`//dockers/docker-fpm-frr:docker-fpm-frr` and `:docker-fpm-frr-dbg` compose
runtime and debug OCI images; their `.gz` targets retain the existing
`docker-fpm-frr:latest` and `docker-fpm-frr-dbg:latest` Docker archive handoff.
The source owners build FRR, its SONiC FPM and SNMP modules, libFIB, bgpcfgd
and frrcfgd. The shared SWSS image supplies SWSS, Common and their source-built
project libraries. The image definitions consume those public TARs directly.

The initial Make selector is native AMD64, Trixie, VS, FIPS, no ASAN, with FRR
SNMP/tcmalloc enabled, the standard `dbg` suffix and existing FRR UID/GID 300. Other profiles retain
their Make path. Existing package declarations, host wrappers and Docker
metadata remain intact; the selected Bazel path depends only on the generated
image manifests. It does not build Debian packages.

The infrastructure production APT lock supplies external runtime libraries and utilities,
including Net-SNMP's Trixie library, tcmalloc, logrotate and lsof. It follows the
parent's exact Debian/security snapshot and trixie-updates selection. It is not
a substitute for any SONiC source producer. The debug image adds the source
FIPS OpenSSH client, its symbols, existing editor alternatives and external
debugger tools. Existing account IDs, configuration ownership, modes, symlinks,
Make manifest labels and installed-package records are explicit image inputs.

Validation stays separate from building:

```sh
python3 tools/bazel/ci/frr_image.py build --artifacts artifacts/frr
python3 tools/bazel/ci/frr_image.py validate --artifacts artifacts/frr
python3 tools/bazel/ci/frr_image.py incremental --artifacts artifacts/frr
```

These commands use the shared CI Bazel entry point and the maintained registry
`main`, consuming the FRR/libFIB producers and shared SWSS image from master.
CI audits the action graph for DEB generation,
verifies runtime and debug contents against owner TARs, checks every source ELF
and its matching symbols, and checks the installed package-state bytes. It
builds both layouts and archives explicitly before recording their digests.
A template edit must change both images/archives while leaving source TARs and
the shared parent unchanged; restoring the input must restore the original
bytes. Equal no-op hashes establish output stability, not an executed-action
cache-hit count.

The separate installed-image job runs as FRR's user with a read-only root,
private temporary files and no external network. It checks the installed hash
and loader closure of every source-owned ELF in each variant, plus the actual Python
entry points, container/wheel templates, Common's table API against a private
Redis instance, and BGPD's Unix VTY using the installed `vtysh`. BGPD disables
privilege changes, Zebra, kernel route updates and BGP/TCP VTY listeners; its
loaded SNMP plugin is verified from the process mappings. Debug-image GDB
checks require the packaged BGPD source lines and types, with a negative
symbol-directory control. These checks do not claim full Docker entrypoint,
Zebra/kernel forwarding, external peer, installer or boot acceptance.

External package requests and their pinned closure belong to
`sonic-build-infra/apt/requests.json` and `apt/apt.lock.json`. The image consumes
its `frr_image_debian`, `frr_image_debug_debian` and `frr_image_test_debian`
sets. Refresh them through the infrastructure unified lock updater, select the
matching module registration, and rerun image and installed-runtime checks.

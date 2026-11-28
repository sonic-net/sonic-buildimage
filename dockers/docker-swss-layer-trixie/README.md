# Shared SWSS image from source owners

This companion to buildimage PR183 consumes the SWSS runtime TAR registered by
[registry PR240](https://github.com/securely1g/sonic-bazel-registry/pull/240), from
[source PR10](https://github.com/securely1g/sonic-swss/pull/10). It replaces PR183's
local libteam Bazel producer and temporary SWSS/Common patches with their shared
owner targets. The Make source checkout, recipes and patch series are unchanged.

| Input | PR183 | This companion |
| --- | --- | --- |
| SWSS | Git source `252a` plus local libteam patch | Registered `0.0.3-9c3797990d4b5a79100a40fe7689927c18212361`, which owns those dependency changes |
| libteam | Local `1.31.sonic-buildimage` producer | `1.31.0.sonic.1` from [registry PR236](https://github.com/securely1g/sonic-bazel-registry/pull/236), with the same upstream 1.31 and 16 SONiC patch bodies |
| Common / Sairedis / DASH API | Common7 plus local ZeroMQ patch / Sairedis3 / DASH4 | Common9 / Sairedis5 / DASH5, required by the selected SWSS source |
| Infrastructure / Distroless | Infrastructure25 / Distroless15 | Infrastructure38 / Distroless21, matching the owner interfaces |
| Common Rust test | Image wrapper around the old source binary | The source owner's real runtime test; Rust-listener wrappers use Common's declared `patchelf` argument |

Local and CI builds resolve modules from registry `main`. The hosted workflows use
the same checksum-verified [source mirror policy](../../tools/bazel/ci/source_mirrors.py)
as the installer, with the repository's Debian mirrors first.

Socat uses the same Debian Trixie source as Make, `1.8.0.3-1+deb13u1`,
including Debian’s security update and the SONiC readline and procan patches.
[Published Bazel module](https://github.com/sonic-net/sonic-bazel-registry/tree/main/modules/socat/1.8.0.3.sonic.1).

The image keeps direct `@libteam` runtime TAR inputs. SWSS's reexport filegroups
are not substituted for those inputs because the debug aspect follows the direct
provider edges. The APT lock already excludes `libteam5` and `libteamdctl0`; it is
unchanged. Package metadata now binds those two libraries to the selected libteam
module and verifies their installed native bytes. Other source TARs retain the
existing package-accounting scope; no new dpkg records are invented.

The existing native archive, debug-pair, parent-payload, dependency, loader,
installed-container and cache checks remain. The incremental probe edits the
selected owner's exported `fpmsyncd/bgp_eoiu_marker.py`, preserves the source fence,
checks initial/no-op/edit/restore phases and restores its exact bytes and mode.
The artifact collector reuses the accepted 128MiB lock bound while retaining the
64MiB limit for other dependency JSON and every secret/schema check.

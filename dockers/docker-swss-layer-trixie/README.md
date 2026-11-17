# Shared SWSS image from source owners

This companion to buildimage PR183 consumes the SWSS runtime TAR registered by
[registry PR240](https://github.com/securely1g/sonic-bazel-registry/pull/240), from
[source PR10](https://github.com/securely1g/sonic-swss/pull/10). It replaces PR183's
local libteam Bazel producer and temporary SWSS/Common patches with their shared
owner targets. The Make source checkout, recipes and patch series are unchanged.

| Input | PR183 | This companion |
| --- | --- | --- |
| SWSS | Git source `252a` plus local libteam patch | Registered `0.0.2-ee5a197adb6ae19c891a8844017d590969a10e4c`, which owns those dependency changes |
| libteam | Local `1.31.sonic-buildimage` producer | `1.31.0.sonic.1` from [registry PR236](https://github.com/securely1g/sonic-bazel-registry/pull/236), with the same upstream 1.31 and 16 SONiC patch bodies |
| Common / Sairedis / DASH API | Common7 plus local ZeroMQ patch / Sairedis3 / DASH4 | Common9 / Sairedis4 / DASH5, required by the selected SWSS source |
| Infrastructure / Distroless | Infrastructure25 / Distroless15 | Infrastructure26 / Distroless16, matching the owner interfaces |
| Common Rust test | Image wrapper around the old source binary | The source owner's real runtime test; Rust-listener wrappers use Common's declared `patchelf` argument |

Local and CI builds resolve modules from registry `main`. The hosted workflows use
the same checksum-verified [source mirror policy](../../tools/bazel/ci/source_mirrors.py)
as the installer, with the repository's Debian mirrors first.

Socat uses `1.8.0.2.sonic.6` from [registry PR242](https://github.com/securely1g/sonic-bazel-registry/pull/242),
with the same upstream 1.8.0.2 and Debian
1.8.0.2-1 archive hashes. Its explicit `tar.xz` archive type supports the
verified content URL, whose path has no archive extension. This upgrade
from `.3` also includes the published compiler-path normalization and
procan namespace-string termination fixes from `.4` and `.5`; executable
and debug bytes may change. Configured features, tools, installed paths
and runtime/debug TAR interfaces remain as declared. Fresh resolution
and native acceptance for this selected version are still required.

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

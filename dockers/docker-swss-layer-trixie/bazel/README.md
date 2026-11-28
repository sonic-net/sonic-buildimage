# Shared native AMD64 SWSS image

`//dockers/docker-swss-layer-trixie:docker-swss-layer-trixie` assembles the shared
SWSS parent directly from the source config-engine OCI image and existing owner
TARs. Its public outputs are:

| Target in this Docker package | Output |
| --- | --- |
| `:source_runtime` | Shared source runtime TAR added over config-engine, also used by Orchagent |
| `:source_debug_symbols` | Matching symbols for that source TAR |
| `:docker-swss-layer-trixie` | Native AMD64 OCI image |
| `:docker-swss-layer-trixie.debug_symbols` | Matching source symbols through the entire parent chain |
| `:docker-swss-layer-trixie.gz` | Reproducible Make-compatible archive tagged `docker-swss-layer-trixie:latest` |

The source TAR combines SWSS's complete 30-program runtime, SAI Redis, SAI
metadata, DASH API, its shared Protobuf ABI, and SONiC's patched libteam and
libteamdctl. Their existing owners compile them once and supply matching symbols.
Common is not repeated: the shared image inherits Common's libraries, Python
binding, source libnl, configuration, database paths and accounts from
config-engine, so each shared object ships exactly once. It adds no service
account.

Selected SWSS source, link arguments and ELF dependencies do not use libFIB;
that decision is checked by the separate SWSS native-closure validation. FRR
retains its own header and Make runtime-content prerequisite. libteam-utils
belongs to teamd, not this shared layer.

The root module selects `iputils-ping`, `libjemalloc2` and `libyaml-cpp0.8`
from the production `orchagent_debian` collection in `sonic-build-infra`.
The shared `apt_inputs` rule follows their locked dependency edges. The
layer's generated lock view supplies the resulting ten AMD64 package records,
including their archive, payload and control hashes, to the standard OCI
selector. It retains packages already in the parent and installs only the
missing payloads. Orchagent consumes the same source runtime TAR for libteam.

`runtime_package_metadata` preserves the parent's truthful installed package
records, records the selected APT additions, and records `libteam5` and
`libteamdctl0` from libteam's recipe-derived identity because their files are
installed in this layer. It does not fabricate SWSS/DASH package identities.
Source file hashes, ELF dependencies and matching symbols are independently
checked against the assembled OCI inventory and the loaded image.

Make's existing archive bridge selects this target only for native AMD64 VS,
Trixie, ASAN disabled, and `BUILD_WITH_BAZEL_WHEN_AVAILABLE=y`. It preserves
legacy declarations for excluded configurations. The selected archive needs
the base/config-engine/shared-layer manifests, not project DEBs or Make's
package cache. Direct preparation and build are:

```sh
make -f tools/bazel/prepare_manifests.mk \
  'MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-swss-layer-trixie.mk' \
  BUILD_WITH_BAZEL_WHEN_AVAILABLE=y BLDENV=trixie CONFIGURED_PLATFORM=vs \
  CONFIGURED_ARCH=amd64 INCLUDE_FIPS=y DOCKERS_PATH=dockers
bazel build //dockers/docker-swss-layer-trixie:docker-swss-layer-trixie.gz
```

CI runs these builds through the shared entry point
`tools/bazel/ci/libteam_draft_bazel.sh`, which loads `tools/bazel/common.bazelrc`
with the canonical registry main, the same catalog ordinary `.bazelrc` uses.

The [Bazel OCI workflow](../../../.github/workflows/bazel-oci.yml) builds this
image in its AMD64 archive job, reusing checkout, execution tools, Bazel setup,
the shared-image disk cache and pinned source preparation. It runs on pull
requests, master pushes and manual dispatches. The job audits the action graph
for DEB generation, checks all source files against the final OCI inventory,
matches every source ELF with its symbols and preserves inherited files. A
dependent runtime job loads that run's exact exported archive and checks
installed hashes and the complete loader closure. A declared installed
Python edit must change the runtime TAR/archive while preserving compiled
symbols and the parent; restoring it must reproduce the original bytes.
This is same-output-base no-op/edit evidence, not a cold-cache compilation claim.

Descendant service fixtures and complete VS installer acceptance remain in their
own integration tasks. The source/OCI-only CI scope does not run Make's full
installer or any DEB-producing target. The Make bridge is registered here; full
installer execution is not claimed by these focused checks.

To refresh these external packages, update the owning production APT lock in
`sonic-build-infra`, select its published registration in the root module,
and rerun the image, loader and no-op/edit checks. The three layer-specific
roots remain in the root module's `swss_layer_apt_inputs` declaration.

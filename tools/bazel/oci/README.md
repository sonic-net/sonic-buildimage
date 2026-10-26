# Shared OCI container tools

## Package contracts and shared tests

`package_contract.py` checks declared package contents and provenance. Container
owners supply expectations; the implementation has no table of container names,
package names, module versions or public targets. SWSS and Syncd use this same
module and select the same package/image regression tests in their `:ci_tests`.
The owner declarations stay beside each container's original Dockerfile and
startup inputs: `dockers/docker-orchagent/` for SWSS and
[`platform/vs/docker-syncd-vs/`](../../../platform/vs/docker-syncd-vs/bazel/README.md)
for Syncd VS. Shared implementations remain in this directory.

Each owner BUILD generates `:package_policy` with `write_file` and `json.encode`.
It reuses the existing APT policy's identity and features, then adds the checks
that APT selection does not describe. The generated JSON is a declared action
and test input, not another checked-in copy of those values.

| Policy field | Purpose |
| --- | --- |
| `debug_tools` | Expected debug package selection. Runtime cannot contain this debug selection. |
| `retained_requirements` | Requirements for imported runtime packages which debug must inherit. For example, when FIPS is enabled, Syncd requires `openssh-client` with `+fips` in its version and the declared `usr/bin/ssh` ELF. |
| `install_inventory` | How to read source-owned install declarations and compare the resulting files, modes and data bytes. SWSS reads its existing C++/Lua declarations and Debian install file; the policy does not copy the program list. |
| `libraries`, `required_debug_paths`, `config_paths` | Required library payloads and SONAME links, matching symbols, and installed configuration metadata. |
| `native` | Required native package/SONAME coverage, explicitly accepted existing symbol gaps, and source/imported symbol requirements. |
| `image_validation` | Expected startup/configuration, archive tags, and explicitly permitted installed-state changes. |

Syncd consumes the public source TAR targets listed in its `BUILD.bazel`.
Each producer owns its installed paths, permissions, ownership and service
links. The generated `source_contract` reads producer metadata and selected
module declarations, checks their target bindings and measured TAR hashes, and
supplies package identity and dependency receipts. There is no image-side
source-path catalog or permission repair. SWSS's source install lists likewise
remain in the SWSS repository.

Compiler, Automake and OpenMPI alternatives are small shared TAR targets in
`sonic-build-infra`, following the existing Vim alternative layer. Canonical
APT locks and generated installed-package status remain build inputs/outputs;
the old installation-state snapshot is unnecessary. Installed-runtime smoke
policies are maintained by the separate validation work in
[buildimage PR #20](https://github.com/securely1g/sonic-buildimage/pull/20).

`package_contract.py` validates source provenance and receipts;
`package_receipts.py` checks original imported DEB identities after upstream
BSD tar extraction. Reusable payload, ELF and image checks are in
`package_payloads.py`, `native_packages.py` and `image_validation.py`.

The common tests cover both owner policies and unrelated package/architecture
fixtures. `swss_package_contract_test` additionally supplies real source-built
archives to the same validator. These checks do not establish guest boot or
forwarding behavior.

The real archive test keeps the workspace's default configuration and records
the SHA256 of each validated input. CI compares those hashes with the exported
artifacts and the public upload list, preventing a different configuration or
changed archive from being published under a successful package-test result.

## Select APT additions for a container

Load `//tools/bazel/oci:apt_layer.bzl` and declare the container policy in its
`BUILD.bazel`. There is no container-specific selector executable or Python
callback:

```starlark
load("//tools/bazel/oci:apt_layer.bzl", "apt_layer")

apt_layer(
    name = "runtime_layer",
    packages = APT_INPUTS["example_debian"]["amd64"],
    lock = ":apt.lock.json",
    dependency_set = "example_debian",
    base = ":base_layout",
    variant = "runtime",
    policy = {
        "schema": 1,
        "image": "docker-example",
        "architecture": "amd64",
        "distribution": "trixie",
        "retained_source": "none",
        "features": {},
    },
)
```

The macro writes `runtime_layer_policy.json` and passes it as a declared
input to `//tools/bazel/oci:select_apt_payloads`. That shared Python tool runs
when OCI layouts and package archives exist: it checks the platform, inventories
the inherited files/packages, applies the policy, invokes dependency/collision
validation in `sonic-build-infra`, and stages selected TARs plus a receipt.
Starlark declares those actions; it cannot inspect their generated OCI/TAR
contents during analysis. Container tests can consume the generated
`:runtime_layer_policy` target to verify the real BUILD policy.

Orchagent uses `retained_source = "none"` because its native payloads are
source-built. For imported Make packages, use `retained_source = "make"`,
provide exact `features`, and pass the generated package JSON as
`retained_manifest`. The selector verifies image, variant, architecture,
distribution, features and complete original package controls before retention.
Debug selection also requires `base_package_metadata` from the exact runtime
selection receipt and matching runtime Make-manifest hash.

Syncd-vs supplies its Make-built FIPS OpenSSH in the runtime manifest. Debug
inherits that package unchanged; the policy has no replacement option. The
container handoff validator rejects missing or ordinary runtime OpenSSH and a
separate OpenSSH package in debug. FIPS applies to both variants. The shared
selector rejects conflicting inherited package controls for every container.

An image can also supply source-built libraries alongside Make imports. Its
declared preparation action combines the original Make manifest with a separate
`source_packages` inventory. Each source record includes reviewed dependency
controls, its owner module/version/commit/target, and the actual input TAR hash;
the document records the original Make manifest and source receipt hashes.
Selection checks dependencies across both inventories and rejects duplicate
ownership. Receipts keep `skipped_source` separate from `skipped_make`, and
debug must match runtime's source receipt. Syncd uses this for the Common,
sairedis and metadata targets already consumed by Orchagent. The image's source
adapter and final-image checks validate the payload and symbol hashes; APT
selection does not claim that these TARs were installed as Debian packages.

The shared tool preserves the existing receipt formats: source-built containers
receive `group`, `skipped_retained`, `image` and `policy_sha256`; Make consumers
receive `variant`, `skipped_make`, `make_manifest_sha256` and
an empty `provided_package_replacements` list for compatibility. Both include
the base manifest digest and complete dependency-check evidence. Shared tests run as
`//tools/bazel/tests:apt_selection_test`; container suites check their declared
policies, package contracts and replacement restrictions.

## Render container manifests

Make renders container manifests with the existing `generate_manifest` in
`rules/functions`. Bazel consumes the resulting JSON as an explicit input and
serializes it into the `com.azure.sonic.manifest` image label. Make remains
responsible for computed variables, conditionals, service-template discovery,
version suffixes and container-specific manifest overlays.

## Use Make's manifests

Register runtime and debug outputs through `SONIC_BAZEL_MANIFESTS`,
`<key>_MANIFEST_IMAGE` and optional `<key>_MANIFEST_SUFFIX` in the container's
owning Make rules. The [Make bridge example](../README.md#example-another-runtime-and-debug-archive)
shows the declarations and archive prerequisites. Each registered key produces
`target/bazel-manifests/<key>/manifest.json`. The shared Make rule runs the
existing generator in a temporary directory, validates the JSON and publishes
it atomically, preserving the previous file on failure and its timestamp when
the bytes are unchanged.

Export these generated source files from the root `BUILD.bazel`, as SWSS does,
then declare one label action for each manifest in the container package:

```starlark
load("//tools/bazel/oci:manifest_labels.bzl", "manifest_labels")

manifest_labels(
    name = "labels",
    manifest = "//:target/bazel-manifests/docker-example/manifest.json",
)
manifest_labels(
    name = "debug_labels",
    manifest = "//:target/bazel-manifests/docker-example-dbg/manifest.json",
)
```

Set each `oci_image` target's `labels` attribute to the corresponding action.
The `manifest_labels` macro accepts one manifest and an optional `out` filename.
Its Python tool validates a JSON object and writes one compact JSON label line,
preserving all manifest fields. The declared input lets Bazel invalidate the
label action whenever Make publishes changed metadata. Runtime and debug labels
have separate actions.

For tool tests or direct Bazel calls, prepare manifests first from explicitly
selected metadata and build settings. For SWSS:

```sh
make -f tools/bazel/prepare_manifests.mk \
  MANIFEST_METADATA=rules/docker-orchagent.mk \
  BUILD_WITH_BAZEL_WHEN_AVAILABLE=y BLDENV=trixie CONFIGURED_PLATFORM=vs \
  CONFIGURED_ARCH=amd64 ENABLE_ASAN=n DBG_IMAGE_MARK=dbg DOCKERS_PATH=dockers
```

This target prepares only the registered manifests. It needs Make, `j2` from
`j2cli`, and `jq`; `MANIFEST_METADATA` selects the owner Make file and any
required includes. Supply the configuration that file needs. Normal image
builds prepare manifests in the slave's full Make context before invoking Bazel.
Native AMD64 and ARM64 helper CI use the same SWSS metadata for this test;
the complete SWSS image remains supported only on native AMD64 Trixie VS.

For startup scripts generated by `sonic-cfggen` in a legacy Dockerfile, use
[`sonic_cfggen_template`](../../../src/sonic-config-engine/README.bazel.md).
It shares execution-tool selection, JSON arguments, declared outputs and the
build-time environment, while running the existing template implementation.

Runtime templates that need device or platform state should remain templates
for the container's startup configuration process.

## Validate

```sh
bazel test //tools/bazel/tests:manifest_labels_test \
  //tools/bazel/tests:manifest_labels_action_test \
  //tools/bazel/tests:cfggen_template_test \
  //tools/bazel/tests:swss_render_test
```

Prepare SWSS manifests with the command above before selecting `swss_render_test`.
Shared Make tests exercise the real generator with unrelated container metadata,
service scopes and overlays. Bazel tests check JSON validation and label encoding;
the SWSS test checks the prepared manifests and generated startup script.

## Use Make's OCI base

Register a base in the container's owning Make rules. The shared
[`tools/bazel/docker.mk`](../docker.mk) include prepares the layout from an
existing Make archive target:

```make
SONIC_BAZEL_OCI_BASES += docker-example-base.oci
docker-example-base.oci_OCI_ARCHIVE = $(TARGET_PATH)/docker-example-base.gz
docker-example-base.oci_OCI_PLATFORM = linux/amd64
```

Keep these declarations under the container's opt-in and build-environment
guards. Each name in `SONIC_BAZEL_OCI_BASES` is an output relative to
`$(TARGET_PATH)` and ends in `.oci`. Each base requires its own complete archive
path and explicit `os/architecture` image platform. Archive prerequisites use
Make secondary expansion, so a recursive `=` assignment can reference variables
defined later. The existing Make archive producer remains responsible for
building or restoring the `.gz`; the shared rule owns the preparation recipe.
Relative subdirectories are supported, such as `bases/docker-example.oci`, and
registered outputs can also be requested as direct Make targets.
Add the resulting `$(TARGET_PATH)/docker-example-base.oci` to the consuming
container's `_BAZEL_DEPENDS` as in the [bridge example](../README.md#example-another-runtime-and-debug-archive).

Non-FIPS SWSS builds use this interface to publish `target/docker-config-engine-trixie.oci`
from the Make-built `target/docker-config-engine-trixie.gz`, with image platform
`linux/amd64`. FIPS SWSS builds and source-built syncd VS consume
`//dockers/docker-config-engine-trixie:docker-config-engine-trixie` directly.
Make passes `--//dockers/docker-orchagent:source_config_engine=true` to both
SWSS archive variants when `INCLUDE_FIPS=y`, and prepares the base and
config-engine manifests instead of extracting the Docker archive. Direct Bazel
reproductions of that configuration must use the same flag and manifests.
Each owner keeps its own configuration guards. The syncd VS
[owner guide](../../../platform/vs/docker-syncd-vs/bazel/README.md) records its
image validation requirements and remaining runtime checks.
The pinned Docker 28.5.2 saves both
Docker metadata and an OCI layout in the same archive. Make extracts the existing
OCI files without changing the index, config or layer bytes. Both outputs
therefore describe the same final image, including Make's version-cache cleanup.

The shared producer is `tools/bazel/oci/prepare_oci_base.py`, also exposed as
`//tools/bazel/oci:prepare_oci_base`. It imports the sibling `oci_layout` validator;
neither helper depends on the SWSS build package.

Declare the prepared layout files as Bazel inputs; the root `BUILD.bazel` does
this for SWSS's config-engine base. The
`oci_base_layout` macro validates descriptor digests, sizes and the requested OS
and architecture, then uses upstream `copy_to_directory` to assemble the directory
consumed by `oci_image`. The check reads the image config even when the index has
no platform descriptor. Its validated `oci-layout` marker is a required input to
the copy action. There is no Docker-to-OCI conversion or layer recompression.

The shared Make rule checks the layout on each request, including when Make's
package cache supplied the archive. It repairs missing or damaged layouts from
that archive, publishes a complete generation atomically, and preserves unchanged
files and timestamps on warm builds. A Docker-only archive from an older cache
fails with instructions to rebuild the base using the pinned Docker version.

Native AMD64 and ARM64 CI exercise the real Make producer and Bazel consumer:

```sh
bazel test //tools/bazel/tests:prepare_oci_base_test \
  //tools/bazel/tests:oci_base_layout_test \
  //tools/bazel/tests:oci_base_consumer_test
```

The fixture starts with a dual-format Docker/OCI archive, publishes its OCI layout
with Make's helper resolved from the fixture's declared runfiles, adds a layer
with `oci_image`, and exports through
`sonic_docker_archive`. Tests check image contents, platform rejection and
reproducibility. The consumer fixture uses an AMD64 image on either execution
host. These targets need no existing Make outputs or Docker daemon and create no
Debian packages.

The [shared Make fixture check](../README.md#shared-bazel-options-and-caches) also
registers cached AMD64 and ARM64 base archives with the production Make include
on both native execution architectures. It prepares both layouts as prerequisites
of container exports and checks unchanged generations, bytes and timestamps on a
repeat request. An archive's image platform comes from its explicit declaration;
this fixture coverage does not establish production ARM64 container support.

## Reproducible Docker archives

`sonic_docker_archive` uses published `rules_gzip` to compress the Docker-save
tar produced by `oci_load`. Its execution toolchain uses pinned `pigz` with
`--no-name`, configured through standard `rules_multirun` and `toolchain_utils`
rules. Compression stays at level 6; source filenames and timestamps are omitted
from the gzip header. A standard Skylib `copy_file` gives each input the basename
needed to retain Make's `<container>.gz` filename. The intermediate copy lives in
a separate directory so it cannot collide with the OCI image directory.

The `//tools/bazel/tests:sonic_docker_archive_test` fixture exercises this macro and
checks its Docker tag, configuration and layer contents, zero gzip timestamp,
absent source filename, and identical compressed bytes for identical tars with
different source filenames and requested modification times. It reports the
observed input times because Bazel may normalize generated-file metadata.

Native AMD64 and ARM64 jobs run this fixture without Make outputs or a Docker
daemon. To run it on native AMD64 Trixie:

```sh
bazel test //tools/bazel/tests:sonic_docker_archive_test
```

On native ARM64 Trixie, also select
`--platforms=@sonic_build_infra//platforms:aarch64_trixie` and
`--host_platform=@sonic_build_infra//platforms:aarch64_trixie`. This test validates
compression and archive structure; it does not build a production ARM64 image.

## Inspect image layers

`oci_inventory.py` applies layer metadata and OCI whiteouts to a filesystem
inventory and checks that added package layers preserve inherited directory
symlinks and links to ELF files. Link resolution uses the image inventory, never
the build host filesystem. Shared payload and image validators use
`normalize_layer.py` for reviewed merged-usr paths. Owners declare their input
layers and package expectations in BUILD; the shared selector and inventory
helpers apply them without image-specific package names or feature settings.

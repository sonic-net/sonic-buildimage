# Buildimage Bazel support

Shared support belongs beside the tools it serves; container-specific targets
and expectations remain beside the container's original Dockerfile and startup
inputs. SWSS is owned by `dockers/docker-orchagent/`; Syncd VS is owned by
`platform/vs/docker-syncd-vs/`, including its `BUILD.bazel`, `bazel/` and
`config/` declarations. Syncd does not need a second directory under `dockers/`.

Tracked scripts, templates and Make metadata are exported by `BUILD.bazel` files
in `files/scripts`, `files/build_templates` and `rules`. The root `BUILD.bazel`
declares the Make-generated manifests, config-engine OCI files and Scapy wheel.

| Location | Responsibility |
| --- | --- |
| `docker.mk` | Shared Make rules for OCI base preparation and opted-in runtime/debug archives: explicit inputs, platforms, Bazel targets, publication and SBOM attribution |
| `manifests.mk` | Register manifest outputs and render them with Make's existing generator before Bazel runs |
| `prepare_manifests.mk` | Prepare only the registered manifests from explicitly selected Make metadata and configuration |
| `build_helpers.py` | Persistent cache options and atomic archive publication, shared by Make recipes and cache checks |
| `cargo-config.toml` | Empty repository-wide Cargo configuration required by the root `rules_rs` configuration |
| [oci/](oci/README.md) | Manifest labels, Make OCI base publication and validation, and Docker archive export |
| [tests/](tests/README.md) | Unit tests and fixture-based integration checks for the Bazel helpers and container build integration |
| [ci/](ci/README.md) | Dependency evidence, command logs, archive/ELF inspection and cache verification |
| [SWSS container](../../dockers/docker-orchagent/bazel/README.md) | SWSS archive selection, Make integration and expected SWSS package contents |
| [syncd VS container](../../platform/vs/docker-syncd-vs/bazel/README.md) | Complete syncd VS runtime/debug OCI packaging and the declared Make package inputs |

The [VS runner and Make cache tests](../ci/README.md) belong to buildimage CI and
do not depend on SWSS helper modules. Shared Python libraries use repository
imports such as `from tools.bazel import build_helpers`. Command-line entrypoints
set their repository import path so Make and CI can invoke them directly.

`build_helpers.py options` prepares optional repository/disk caches and prints
one argument per line for a Bash array; `--arguments` accepts shell-quoted,
single-line options without evaluating shell commands. `build_helpers.py export`
validates a single nonempty archive from `--query-output` and publishes it
atomically to `--output`, retaining timestamps when its bytes are unchanged.
The shared `docker.mk` recipe owns the Bazel `build` and `cquery` calls.

## Opt a container into the Make bridge

`BUILD_WITH_BAZEL_WHEN_AVAILABLE` is the shared Make switch and defaults to `n`.
With `y`, each container's Make rule registers its Bazel runtime and debug
archives only when the selected build configuration supports them. Registered
archives use Bazel; all other containers keep their existing Make Docker build.
With `n`, all containers use that existing Make path. The same switch also
automatically selects the registered kernel DEB build for supported native
AMD64/ARM64 Trixie VS unsigned default configurations; see the
[kernel integration](kernel/README.md).

Availability means that the container owner has supplied Bazel targets and
enabled them for the selected architecture, platform and features. Make does
not probe for a Bazel executable or retry a failed Bazel build with Make.
Invalid metadata for a selected target and Bazel build errors remain failures.

Keep the container's Make declarations in their existing location, such as
`rules/docker-orchagent.mk` or `platform/vs/docker-syncd-vs.mk`.
Select Bazel only under that owner's feature and build-environment guards. The
shared bridge in `tools/bazel/docker.mk` consumes these declarations:

| Declaration | Meaning |
| --- | --- |
| `SONIC_BAZEL_DOCKER_IMAGES` | Runtime archive names, such as `docker-example.gz` |
| `SONIC_BAZEL_DBG_DOCKER_IMAGES` | Debug archive names, such as `docker-example-dbg.gz` |
| `SONIC_BAZEL_SWITCHABLE_IMAGES` | Runtime/debug archive names that can switch builders; register these even when their Bazel selector is off |
| `$(archive)_BAZEL_TARGET` | Required explicit Bazel label producing one nonempty archive file |
| `$(archive)_PATH` | Required container source path used for the archive's SBOM attribution |
| `$(archive)_BAZEL_DEPENDS` | Optional Make prerequisites, expressed as complete paths and expanded when Make evaluates the archive's dependencies |

Declare the target and path for each archive, including the debug archive. The
bridge does not derive a Bazel package or target name from the archive filename.
`_BAZEL_DEPENDS` can reference variables defined later, using a recursive `=`
assignment. Include Make-produced inputs such as an OCI base layout or a wheel;
the bridge does not prepend `target/`, a package directory or a wheel directory.
Provide the Make rules that produce those inputs before using the archive.
For an OCI layout extracted from an existing Make Docker archive, register the
base with the same shared include:

| Declaration | Meaning |
| --- | --- |
| `SONIC_BAZEL_OCI_BASES` | OCI output names relative to `$(TARGET_PATH)`, such as `docker-example-base.oci` |
| `$(base)_OCI_ARCHIVE` | Required complete Make path to the existing Docker archive; expanded when Make evaluates the base's dependencies |
| `$(base)_OCI_PLATFORM` | Required image platform, such as `linux/amd64` |

The container owner supplies these values under its existing selection guards.
The shared rule depends on the archive producer and invokes the OCI preparation
helper, including after a Make package-cache hit. Use a recursive `=` assignment
for an archive path whose variables are defined later. The platform describes the
image in the archive, independently of the machine running Make. Registered
layouts are also direct Make targets, such as `target/docker-example-base.oci`.
See the [OCI base guide](oci/README.md#use-makes-oci-base) for validation and atomic reuse.

For manifests, the bridge includes `tools/bazel/manifests.mk`. Register each
output independently:

| Declaration | Meaning |
| --- | --- |
| `SONIC_BAZEL_MANIFESTS` | Manifest keys, each published as `$(TARGET_PATH)/bazel-manifests/<key>/manifest.json` |
| `$(key)_MANIFEST_IMAGE` | Required archive name whose existing Make metadata supplies the manifest, such as `docker-example.gz` |
| `$(key)_MANIFEST_SUFFIX` | Optional version suffix, such as `dbg` |

Make evaluates the owner's metadata and calls `generate_manifest` from
`rules/functions`, including service discovery and `manifest.part.json.j2`
overlays. Both variants normally select the runtime archive's metadata. Their
separate output directories let runtime and debug prerequisites run together.
Each request checks the generated JSON and publishes changed bytes atomically;
unchanged bytes retain their timestamp. Add the files to `_BAZEL_DEPENDS` and
declare them as Bazel inputs. A debug image that extends the runtime image needs
both manifests. The [manifest guide](oci/README.md#use-makes-manifests) shows how
to turn each JSON input into OCI labels.

Keep the existing `SONIC_DOCKER_IMAGES`, `SONIC_DOCKER_DBG_IMAGES`, release-specific
selectors and installer lists, plus container, manifest, package and load metadata.
Other Make consumers can still need them. The bridge removes opted-in archives
from the legacy Docker build recipes while preserving target and load integration.
The owner retains its architecture/feature checks. For each switchable archive,
the bridge records `make` or `bazel` in
`$(TARGET_PATH)/.container-build-method/<archive>`. A new or changed stamp
invalidates an existing archive, including when Make package caching is disabled;
unchanged selections preserve the stamp timestamp. Keep the owner's selector in
its existing `_DEP_FLAGS` as well, so package-cache keys distinguish the builders.
When returning to Make, the cache loader must replace the Bazel archive even if
the destination already exists. The shared bridge requests a staged restore of
that archive from a matching Make cache; a failed extraction preserves the prior
archive and leaves the newer stamp in place for a retry.
Archives selected for Bazel cannot appear in `SONIC_PACKAGES_LOCAL`: the shared
archive macro supplies `:latest`, while that Make mode expects versioned tags.
SWSS supports native AMD64 Trixie VS without ASAN. Other SWSS configurations
keep the Make build even with `BUILD_WITH_BAZEL_WHEN_AVAILABLE=y`.
The syncd VS owner also registers an opt-in for native AMD64 Trixie VS with
DASH SAI and FIPS enabled, without ASAN, RPC, or a combined `docker-sonic-vs`
target. Its [owner guide](../../platform/vs/docker-syncd-vs/bazel/README.md) records
the current image validation requirements and remaining runtime checks.
The config-engine and py-common wheel targets are separate, directly invoked
Bazel targets; this switch selects container builders.

### Example: another runtime and debug archive

This example assumes the container already has runtime/debug `oci_image` targets
and a Make producer for `target/docker-example-base.gz`. In its `BUILD.bazel`,
use the standard archive macro and, if useful, separate labels for the Make entry
points:

```starlark
load("//tools/bazel/oci:sonic_docker_archive.bzl", "sonic_docker_archive")

sonic_docker_archive(
    name = "docker-example.gz",
    image = ":runtime_image",
)
sonic_docker_archive(
    name = "docker-example-dbg.gz",
    image = ":debug_image",
)
alias(name = "make-runtime-export", actual = ":docker-example.gz")
alias(name = "make-debug-export", actual = ":docker-example-dbg.gz")
```

Each archive target has one file in its default outputs. The macro supplies the
Docker-save format, reproducible gzip and `<archive stem>:latest` tag expected by
Make. The debug `oci_image` should extend the matching runtime image. See the
[archive guide](oci/README.md#reproducible-docker-archives).

Add the following to the container's existing Make rule, alongside its supported
platform and feature checks. This example assumes its Bazel targets support
native AMD64 Trixie builds. Keep its legacy selectors and metadata in place:

```make
DOCKER_EXAMPLE = docker-example.gz
DOCKER_EXAMPLE_DBG = docker-example-$(DBG_IMAGE_MARK).gz

$(DOCKER_EXAMPLE)_PATH = $(DOCKERS_PATH)/docker-example
$(DOCKER_EXAMPLE_DBG)_PATH = $($(DOCKER_EXAMPLE)_PATH)

ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_EXAMPLE) $(DOCKER_EXAMPLE_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_ARCH),y:amd64)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON)),)
SONIC_BAZEL_OCI_BASES += docker-example-base.oci
docker-example-base.oci_OCI_ARCHIVE = $(TARGET_PATH)/docker-example-base.gz
docker-example-base.oci_OCI_PLATFORM = linux/amd64
SONIC_BAZEL_MANIFESTS += docker-example docker-example-dbg
docker-example_MANIFEST_IMAGE = $(DOCKER_EXAMPLE)
docker-example-dbg_MANIFEST_IMAGE = $(DOCKER_EXAMPLE)
docker-example-dbg_MANIFEST_SUFFIX = dbg
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_EXAMPLE)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_EXAMPLE_DBG)
$(DOCKER_EXAMPLE)_BAZEL_TARGET = //dockers/docker-example:make-runtime-export
$(DOCKER_EXAMPLE_DBG)_BAZEL_TARGET = //dockers/docker-example:make-debug-export
$(DOCKER_EXAMPLE)_BAZEL_DEPENDS = $(TARGET_PATH)/docker-example-base.oci \
    $(TARGET_PATH)/bazel-manifests/docker-example/manifest.json
$(DOCKER_EXAMPLE_DBG)_BAZEL_DEPENDS = $($(DOCKER_EXAMPLE)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-example-dbg/manifest.json
endif
endif
endif
```

The base declaration prepares `target/docker-example-base.oci` through the shared
rule; no container-specific preparation recipe is needed. Declare those layout
files as Bazel inputs and consume them with `oci_base_layout` as described in the
[OCI guide](oci/README.md#use-makes-oci-base).

Make can then request either container archive through the shared recipe. Each request
lets Bazel check its declared inputs, queries the selected label's output, and
publishes it atomically under the existing Make archive name. An unsuccessful
build or query preserves the previous archive; identical bytes preserve its
timestamp. Add declarations for another container without copying this export
recipe.

## Shared Bazel options and caches

Make uses `SONIC_BAZEL_CACHE_SOURCE` as the host directory for shared Bazel
caches. Its default is `$(SONIC_DPKG_CACHE_SOURCE)/bazel`, independent of
`BUILD_WITH_BAZEL_WHEN_AVAILABLE`. Set it to a persistent directory writable by
the builder user to override that default. Make creates the directory and
checks that it is writable before starting Docker.

Make mounts this directory at `/bazel_cache` and mounts
[`slave.bazelrc`](slave.bazelrc) at `/etc/bazel.bazelrc`. Bazel reads that system
configuration automatically:

```text
common --repository_cache=/bazel_cache/repository_cache
common --disk_cache=/bazel_cache/disk_cache
```

This applies to ordinary Bazel commands inside the builder, including P4RT and
the container bridge. Downloaded repositories and completed action results are
shared; each builder keeps its own output directories and Bazel server. The
configuration does not share `output_user_root`. Bazelisk stores downloaded
Bazel binaries in `/bazel_cache/bazelisk`.

An explicitly empty `SONIC_BAZEL_CACHE_SOURCE` omits both this cache mount and
the mounted system configuration. Bazel still runs with its usual local
configuration; an empty value does not disable Bazel or its own local caches.

Inside the builder, `BAZEL` selects the executable or launcher. The bridge accepts:

| Environment variable | Meaning |
| --- | --- |
| `BAZEL_CONTAINER_ARGS` | Additional shell-quoted, single-line options passed to both `build` and `cquery` |
| `BAZEL_CONTAINER_CACHE_DIR` | Optional cache override for bridge commands; repositories use `repository_cache/` and completed actions use `disk_cache/` |

These variables take precedence when set, even when empty. When unset, the bridge
accepts the existing `BAZEL_SWSS_ARGS` and `BAZEL_SWSS_CACHE_DIR` respectively for
compatibility. An explicitly empty generic value suppresses that legacy fallback;
independent rc settings still apply, including `/etc/bazel.bazelrc` when mounted.
Make does not automatically export `BAZEL_CONTAINER_CACHE_DIR`: the system
configuration supplies the normal shared caches. Set the variable only to
override those paths for bridge commands, for example
`BAZEL_CONTAINER_CACHE_DIR=/tmp/bazel-example-cache`.
Options are parsed without evaluating shell commands. For example, inside a
native Trixie builder:

```sh
BAZEL_CONTAINER_ARGS='--jobs=4' \
BAZEL=bazel make -f slave.mk target/docker-example.gz \
  BUILD_WITH_BAZEL_WHEN_AVAILABLE=y
```

The normal Make platform and build-environment setup must already be selected.
When invoking Make outside the builder, forward extra options through
`SONIC_BUILDER_EXTRA_CMDLINE`.
See the SWSS [cache example](../../dockers/docker-orchagent/bazel/README.md#reuse-local-build-caches).

Run the [Bazel helper unit tests](tests/README.md) with:

```sh
python3 -B -m unittest discover -s tools/bazel/tests -p '*_test.py'
```

Native AMD64 and ARM64 archive CI also exercise the production Make include with
real Bazel archive and OCI-consumer fixtures. The check prepares two cached base
archive fixtures, one AMD64 and one ARM64, as prerequisites of three exports
across two container families and both runtime/debug selector lists. It repeats
the requests and verifies unchanged layout generations, file hashes, publication
link timestamps and exported archive hashes/timestamps. Each base's declared
image platform is checked independently of the native execution host. The OCI
consumer fixture continues to use a fixed AMD64 image. This checks preparation,
bridge routing and publication; the debug selector uses a fixture without production
debug symbols. Reproduce it with:

```sh
python3 tools/bazel/ci/verify_make_archive.py \
  --artifacts artifacts/archive/make-export --bazel bazel
```

On native ARM64 Trixie, append
`--bazel-arg=--platforms=@sonic_build_infra//platforms:aarch64_trixie` and
`--bazel-arg=--host_platform=@sonic_build_infra//platforms:aarch64_trixie`.

## Registry selection

Local commands default to the maintained SONiC registry `main` endpoint from
`.bazelrc`, alongside Bazel Central Registry. Module versions, source commits,
checksums and package snapshots remain pinned.

`rules_distroless` 0.9.4.sonic.1 is available on the maintained registry after
[registry #49](https://github.com/securely1g/sonic-bazel-registry/pull/49) landed.
CI and local validation use the same `main` endpoint. The dotted version sorts
above plain 0.9.4, so Distroless needs no root version override.

```sh
bazel test //tools/bazel/tests:select_apt_payloads_test
```

The shared APT baseline was published by
[registry #45](https://github.com/securely1g/sonic-bazel-registry/pull/45) from
[build-infra #27](https://github.com/securely1g/sonic-build-infra/pull/27).
Declarative policy inputs originated in
[build-infra #29](https://github.com/securely1g/sonic-build-infra/pull/29).
The image root now selects published `sonic-build-infra`
`0.0.23-5b224d7190d04f1b7b68429eedfc03b7b1565ed0` from the maintained registry.
Its `single_version_override` keeps transitive consumers on that same registered
version; no temporary infrastructure source override is needed. Commit suffixes
do not sort by source history, so verify resolution when updating this pin.

Use `bazel help --announce_rc` to inspect the effective rc selection. Keep one
SONiC registry endpoint; do not add another endpoint in a home or user rc.
Generated resolution receipts list registry lookup URLs from the lock; a reused
lock can also contain historical lookups.

## Kernel Debian packages in the Make VS flow

The opt-in kernel runtime/debug/header DEB producer and installer handoff are documented
in [kernel/README.md](kernel/README.md). The separate kernel workspace keeps
the source/cache graph independent of the container graph and verifies the DEBs
against the current buildimage inputs and kernel gitlink. Packages go directly
to the normal `target/debs/trixie` directory.

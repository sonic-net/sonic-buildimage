# Build the VS Syncd container with Bazel

`BUILD_WITH_BAZEL_WHEN_AVAILABLE=y` selects the registered Bazel runtime and
debug images without filtering platform, architecture or FIPS settings. For
example, `CONFIGURED_ARCH=arm64 INCLUDE_FIPS=n` selects the same Bazel targets
as AMD64 with FIPS enabled. Setting the global flag to `n` keeps Make/Docker.

The remaining guards still require Trixie, DASH enabled and the `dbg` image
marker, with no cross build, QEMU, ASAN or Syncd RPC; builds of the
`docker-sonic-vs` container also keep their existing producer. This selection
policy does not add a toolchain or establish image validation for every tuple.

Bazel builds the project runtime packages from source, starting with the
`docker-base-trixie` and `docker-config-engine-trixie` images. PI, BMv2, P4C and
DASH SAI use their existing Autotools or CMake builds under Bazel and
produce runtime/debug TARs directly. Existing Make package recipes remain
available and unchanged.

Autoconf, Automake, Libtool, Make, M4 and pkg-config are pinned Debian execution
tools supplied by `sonic-build-infra`. The foreign builds use its declared GCC
toolchain and tool runtimes. The selected `rules_foreign_cc` patch preserves
ordinary, system and quote header search paths when invoking Make/configure;
vendor headers therefore retain their intended warning and lookup behavior.
The shared tool launcher stays a static C binary under GCC and Clang. DASH
metadata generation uses the declared spelling tool from the pinned runtime.

## Build

The default registry configuration uses maintained `main` plus Bazel
Central Registry. `MODULE.bazel` selects the published versions of these
producer updates:

| Module | Published registration | Producer change |
| --- | --- | --- |
| Common | [registry #153](https://github.com/securely1g/sonic-bazel-registry/pull/153) | Data permissions, canonical database path and library-link modes |
| Sairedis | [registry #154](https://github.com/securely1g/sonic-bazel-registry/pull/154) | Syncd service links in the runtime TAR |
| FIPS | [registry #152](https://github.com/securely1g/sonic-bazel-registry/pull/152) | OpenSSH agent permissions/ownership and socket link |
| Build infrastructure | [registry #151](https://github.com/securely1g/sonic-bazel-registry/pull/151) | Compiler, Automake and OpenMPI alternatives TARs |

The registry recipes pin landed source archives and patches by checksum.
Generated resolution receipts record the selected module versions. Local
builds and CI use the same registry configuration.

Use the existing Make interface after configuring native VS:

```sh
make target/docker-syncd-vs.gz target/docker-syncd-vs-dbg.gz \
  BUILD_WITH_BAZEL_WHEN_AVAILABLE=y INCLUDE_VS_DASH_SAI=y INCLUDE_FIPS=y \
  ENABLE_ASAN=n ENABLE_SYNCD_RPC=n
```

Make prepares the original image manifests and selects `--config=syncd-dash`
for both Syncd archive targets. That configuration enables the source owner's
DASH daemon and supplies `@sonic_dash//:sdk`. No P4 DEB file is an OCI prerequisite.
Other containers keep their own settings. Both the build and output query use
the same configuration.

After those inputs exist, the direct targets are:

```sh
bazel build --config=syncd-dash \
  //platform/vs/docker-syncd-vs:docker-syncd-vs.gz \
  //platform/vs/docker-syncd-vs:docker-syncd-vs-dbg.gz
```

## Runtime and debug inputs

| Input | Producer |
| --- | --- |
| Base filesystem, FIPS libraries, Python runtime and common service tools | The source-built `docker-config-engine-trixie` graph; ordinary Debian packages remain pinned APT inputs. |
| Common and its Rust CLI library | `@sonic_swss_common//dist:libswsscommon_pkg` |
| sairedis and SAI metadata | `@sonic_sairedis//lib:libsairedis_pkg` and `//meta:libsaimetadata_pkg` |
| VS SAI | `@sonic_sairedis//vslib:libsaivs_pkg` |
| Syncd and the DASH-enabled daemon | `@sonic_sairedis//syncd:syncd_pkg` |
| FIPS OpenSSH client | `@sonic_fips//openssh:openssh_client_pkg` |
| libnl development files | Source-owned headers, static libraries and link/pkg-config files matching the two Make development packages. |
| PI | `@p4lang_pi//:runtime` |
| BMv2 | `@p4lang_bmv2//:runtime` |
| P4C | `@p4c//:p4c_pkg` |
| DASH SAI | `@sonic_dash//:runtime` |

[BUILD.bazel](../BUILD.bazel) lists the actual target bindings. The generated
`source_contract` reads each producer's own `runtime_packages.json`, the selected
`MODULE.bazel` and the exact runtime TAR. Producer metadata supplies package
identity and any producer-declared file checks; the image checks their payload hashes
and consumes the TARs' file modes and ownership directly.
The `syncd-dash` configuration selects both the DASH SDK and its generated
metadata for sairedis dependency analysis. SONiC owners use their
full source commit; foreign packages use the upstream release and SONiC patch
version. Assembly checks the resolved module declarations and actual TAR bytes.

The debug image extends the exact runtime image. Its source symbols come from
the same Bazel dependency graph, including the source-built base and all four
P4/DASH producers. The package policy requires their runtime/debug ELF pairs;
there is no exemption for missing P4 symbols. Static development libraries
retain their own symbols. A successful build is not a FIPS certification claim.

On AMD64, `p4testgen` also links the producer's declared retained Z3 static
library. Those Z3 objects have no DWARF; matching symbols cover the rebuilt
P4C/testgen code. The other shipped P4C programs do not link Z3, and ARM64
disables testgen.

PI's `pi_CLI_dummy` launcher imports `main` from `libpicli.so.0`; it has no
source objects of its own. Validation checks the launcher's build ID and
debug link, its imported function and required library, and the provider's
matching DWARF. Installed GDB must resolve that function to `main.c` and fail
the same source lookup when the packaged symbols are unavailable.

The checked source receipt binds the reviewed controls, resolved module
identities, original TAR hashes and installed files. Both APT
selection and installed package metadata consume that receipt directly.
The runtime never depends on a debug TAR; the debug layer adds paired symbols
to the exact runtime image.

## APT and installed package state

Both containers use the common `apt_layer`, `sonic_layer`, `oci_image`,
`debug_symbols_layer` and `sonic_docker_archive` rules. Distroless provides package
imports and TAR composition. Ordinary Debian dependencies remain declared APT
inputs; Bazel produces no Debian package and runs no maintainer script.

The APT lock uses the same Debian snapshot as the source-built base. For example,
`libc6-dev` must match the base's `libc6` version. The selector also preserves
source-built FIPS libraries when they already satisfy a dependency. It fails if
the final package requirements cannot be satisfied.

The installed package metadata helper combines the base, selected APT controls,
source-package controls and their checked receipts. It checks dependencies again
and writes the final `/var/lib/dpkg/status` with source/archive provenance.
The debug selector checks that exact runtime inventory.

OpenSSH installation preserves the reviewed base groups and adds `_ssh` with
GID 104, matching the retained Make image. The OpenSSH producer's TAR supplies
`ssh-agent` as root:`_ssh` with mode `2755`; the image does not rewrite its
metadata. Source-owned TARs also supply service enablement links. The image
configuration retains the `x /tmp/ssh-*` tmpfiles exclusion.

Compiler, Automake and OpenMPI alternatives come from
`@sonic_build_infra//deps/compiler:alternatives_pkg`,
`@sonic_build_infra//deps/automake:alternatives_pkg` and
`@sonic_build_infra//deps/openmpi:alternatives_pkg`. The image no longer carries
nine legacy `/var/lib/dpkg/alternatives` snapshot records or two duplicate
copyright aliases; canonical license files remain. Generated `/var/lib/dpkg/status`
and source package metadata remain part of the image. The original
`Dockerfile.j2`, `start.sh`, `supervisord.conf`, `critical_processes` and Make
image manifest remain the behavioral reference.

The checked-in `apt.lock.json` is the only APT package list consumed by a build.
`apt.import_lock` reads its package URLs, checksums, architectures and dependency
edges during repository setup and exposes the standard Distroless package targets.
To refresh it, use `apt-resolve.MODULE.bazel` in a private preparation checkout
and review the resulting lock against the base. No expanded import file needs
to be checked in or regenerated.

## Validation

Contract tests exercise selection, source provenance, installed files, package
state and Make fallback behavior. Syncd CI also builds the actual installed-package
status and receipt from the producer TARs with `--config=syncd-dash`; the public
artifacts retain both outputs. Source-owner and base-image workflows validate
their supported native AMD64 and ARM64 configurations separately.
Full DASH Syncd image validation currently runs on native AMD64; native ARM64
source-producer checks do not establish an ARM64 DASH image.

Installed-container validation is maintained separately in
[PR #20](https://github.com/securely1g/sonic-buildimage/pull/20). It uses the
exported runtime/debug images to resolve libraries, exercise CLI and startup
behavior, check GDB symbol lookup, and compile a small P4 program with the
installed driver before starting its local BMv2 listener. These probes are not
part of the image build or the native archive tests.
The full VS workflow retains the existing installer boot and login checks.
Strict guest service validation is maintained separately in
[PR #28](https://github.com/securely1g/sonic-buildimage/pull/28).

Packet forwarding requires a connected test topology and is a separate check.
Building an installer or passing helper tests does not establish forwarding or
guest service health.

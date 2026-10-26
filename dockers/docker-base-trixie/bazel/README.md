# Source-built Trixie base images

These targets build `docker-base-trixie` and `docker-config-engine-trixie` from a
pinned Debian seed, reviewed Debian packages, and the public source-build targets
owned by SONiC components. For example, changing Common's C++ source rebuilds its
runtime TAR and the config-engine image layer; it does not rebuild Debian.

## Public targets

| Owner package | Target | Output |
| --- | --- | --- |
| `//dockers/docker-base-trixie` | `:docker-base-trixie` | OCI image |
| `//dockers/docker-config-engine-trixie` | `:docker-config-engine-trixie` | OCI image using the source-built base |
| Either owner | `:<image>.gz` | Make-compatible Docker archive |
| Either owner | `:<image>.debug_symbols` | Matching source ELF debug files under `/usr/lib/debug/.build-id` |
| Either owner | `:runtime_package_metadata` | Installed package versions, source revisions and native file hashes |

OCI means the standard container image layout consumed by `rules_oci`. A TAR is
the filesystem archive produced by a component's packaging target. These builds
create TARs, wheels and container images; they do not create DEB packages.

## Image assembly structure

The base, config-engine and VS Syncd images use the same structure:

1. Select Debian packages with `apt_layer`, then use `sonic_layer` to remove
   duplicate paths and filter package-state files.
2. Add the source owners' runtime TARs and Python wheels.
3. Add configuration and package metadata, in an explicit order in `oci_image`.
4. Collect matching source debug symbols and export the Make-compatible archive.

Each image's `config/BUILD.bazel` owns its configuration files and Make-generated
labels. The base also needs accounts, certificates and Vim command links there;
config-engine and Syncd inherit that setup. The shared account rule reads the
actual base account files and adds only Redis (UID 100, GID 101); Syncd adds its
`_ssh` group (GID 104). It rejects conflicting names and IDs. Keep package selection, source
targets and the ordered image layers visible in the image's main `BUILD.bazel`.
The previous configuration target names remain aliases for existing consumers.

For example, to add a base configuration file, export the file in the image's
main `BUILD.bazel` and add its destination and mode in `config/BUILD.bazel`.
Adding a source runtime package still belongs in the main image definition and
its owner's package-to-TAR mapping in `BUILD.bazel`. Config-engine's producers
declare their required package files through `runtime_packages(required_paths = ...)`.
Its `bazel/source_paths.json` adds only the image-specific database-config location. `sonic_layer` removes
repeated directory entries from Debian archives before applying the file filter;
Docker rejects repeated paths within a layer.

## Runtime component ownership

`docker-base-trixie` consumes the source packages for socat, iproute2 and SONiC's
OpenSSL, Kerberos and SymCrypt changes. OpenSSL's development files remain in this
base because Make installs `libssl-dev` here. Debian's separately packaged legacy
OpenSSL provider remains an imported APT dependency.

`docker-config-engine-trixie` adds Common's C++ library, database CLI and Python
binding; libnl; libyang and its Python binding; eventd; the Rust supervisor
listener; the config-engine, py-common, YANG management, containercfgd and Python
supervisor wheels; and the source-owned YANG models. The models package retains
its Python distribution metadata and both Python and CVL model directories, so
pip in Make-built child containers can satisfy config-engine's model dependency.
Eventd's separate rsyslog
plugin package is not an input to this image in Make and is not added here.
Libnl uses its precise runtime TARs, preserving `/etc/libnl-3` configuration and
CLI plugins while leaving development linker symlinks to the development TARs.

The shared image path normalizer places Common's database configuration under
`/run/redis/sonic-db`, preserving Debian's `/var/run -> /run` link. The original
component TARs remain graph inputs so this adaptation keeps their debug symbols.
CI rejects payload paths that traverse the preserved directory aliases.
Source-owned paths receive root ownership, and the database configuration uses
mode `0644`, so a developer checkout's uid or umask cannot leak into the image.

The base keeps Make's `vim-tiny` selection and hard link at `/usr/bin/vim`.
It declares the `editor`, `ex`, `rview`, `vi`, and `view` alternatives from the
pinned package's installation script, with each command resolving to
`/usr/bin/vim.tiny`. Debug images may replace these alternatives with full Vim.

Each native producer keeps its own runtime/debug split. The image's debug target
collects those providers through the base chain. SymCrypt's integrity-sealed core
uses build-ID lookup only: adding a debug-link section after sealing would change
the protected bytes. Its provider and engine use ordinary debug links and CRC
checks, as do the other native source packages.

The interpreter in the retained Make container was ordinary Debian Python,
not SONiC's host-image FIPS Python build. It remains a pinned Debian input here.
The five Python 3.13 packages use `3.13.5-2+deb13u4`; glibc uses
`2.41-12+deb13u3`, and Perl uses `5.40.1-6`. The Debian seed is the immutable
`trixie-20260713` manifest in `MODULE.bazel`; APT uses the shared July 2026
snapshots. Downstream exact-version development packages must use that same set.

Third-party Python dependencies use the one checked lock in
`src/sonic-config-engine/requirements_lock.txt`. This replaces the Make
Dockerfile's floating `pip install` selection with reviewed versions. Do not
claim byte-for-byte equivalence to a later floating Make build.

## Build with published producer dependencies

Local builds and CI resolve the pinned Common, build-infra, libnl3, libyang and
libyang-python modules through the maintained SONiC registry and BCR. Their
source archives, patches and overlays are verified by Bazel using the published
registry recipes; no source-preparation step or temporary module override is
needed.

```sh
make -f tools/bazel/prepare_manifests.mk \
  'MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk' \
  BLDENV=trixie CONFIGURED_PLATFORM=vs CONFIGURED_ARCH=amd64 INCLUDE_FIPS=y \
  BUILD_WITH_BAZEL_WHEN_AVAILABLE=y DOCKERS_PATH=dockers

bazel build //dockers/docker-config-engine-trixie:docker-config-engine-trixie.gz
```

If this checkout previously used the draft source preparation helper, remove
only its generated `.bazelrc.user` before building. Its first line identifies
`tools/bazel/ci/prepare_draft_sources.py` as the generator. Preserve manually
maintained user settings. CI starts from a clean checkout and retains the
resolved module graph and generated lock as build evidence.

Sairedis, FIPS, socat and iproute2 also use published registrations. The images
consume runtime and debug TARs without producing DEBs.

## Make selection and direct ARM64 validation

For Trixie builds, `BUILD_WITH_BAZEL_WHEN_AVAILABLE=y` selects these two
archives regardless of `CONFIGURED_PLATFORM`, `CONFIGURED_ARCH` or `INCLUDE_FIPS`.
Setting the flag to `n` keeps the Make producer. The separate cross-build, QEMU
and ASAN guards are unchanged. The Make bridge passes matching target and host
platforms for native AMD64 and ARM64 builds. Other architectures fail with a clear
unsupported-architecture error before building or publishing an archive. Direct
native ARM64 validation selects both target and host platform:

```sh
bazel build \
  --platforms=@sonic_build_infra//platforms:aarch64_trixie \
  --host_platform=@sonic_build_infra//platforms:aarch64_trixie \
  //dockers/docker-config-engine-trixie:docker-config-engine-trixie
```

Prepare Make manifests with `CONFIGURED_ARCH=arm64` for that invocation.
The selector tests cover platform, architecture and FIPS combinations; actual
source/image CI runs on native AMD64 and ARM64. Full Syncd image integration
remains downstream of these base-image targets.

## Package state and validation

The shared `@sonic_build_infra//oci:installed_packages.bzl` rule preserves
Debian control fields from the selected APT packages and reads source package
identity from each producer's generated `runtime_packages.json` target. Its
existing Debian control and changelog supply package fields and versions; the
shared rule derives library dependencies from the actual runtime TARs and
declared target libraries. `BUILD.bazel` maps Debian package names to those TARs.
Config-engine's producers validate their required package files before emitting
metadata. For example, Common requires `libswsscommon.so.0` and its raw
`var/run/redis/sonic-db/database_config.json`; a missing file fails the producer
metadata action even during an ordinary image build. The in-tree eventd and
Rust supervisor targets use the same rule.

Config-engine adds only `run/redis/sonic-db/database_config.json` in its
`source_paths.json`: this image must normalize the producer's `/var/run` path to
`/run`, retain root ownership and mode `0644`, and preserve Debian's
`/var/run -> /run` link. Final-image and installed-runtime checks still verify
those properties and libnl's configuration files after assembly. The base
image retains its existing path policy for the separate base-package producers.

The helper binds each record to its actual TAR label and selected owner module,
checks required files and records native file hashes and ownership. It validates
the final Debian dependency relationships before writing `/var/lib/dpkg/status`
and the receipt under `/usr/share/sonic/bazel`. Architecture comes from the build.
For sources in this workspace, the receipt says `workspace`; CI records the exact
checkout separately. No frozen image-side SHA claims to identify current source.

No package maintainer script runs on the build host. These records describe the
image inventory; they do not create a DEB or perform a package-manager transaction.

`Bazel source base images` builds both architectures, verifies source hashes in
the assembled image, checks all source ELF/debug pairs, and runs the installed
config-engine image in a network-isolated container. It exercises cfggen
rendering, valid and invalid YANG data, Python imports, command scripts, dynamic
library loading, the legacy provider, and actual SymCrypt provider/engine hashes.
The owner's original suites remain in their separate producer workflows.

## Refreshing APT and Python inputs

Keep one reviewed `apt.lock.json` for the base, config-engine and base-debug
package sets. `apt-resolve.MODULE.bazel` contains the small handwritten package
requests used only to refresh that lock. Resolve those requests with Distroless,
then review versions, architecture, dependencies and archive hashes.

Normal builds call `apt.import_lock(lock = "//dockers/docker-base-trixie/bazel:apt.lock.json")`.
Distroless imports the exact locked archives and exposes every package's public
payload and control targets without resolving current repository indexes. The
shared `apt_inputs` repository derives BUILD labels from the same lock. Neither
an expanded `apt_inputs.MODULE.bazel` nor a second hand-maintained package list
is needed.

For Python changes, edit the owning `requirements.in` and regenerate the shared
config-engine lock with the existing documented `uv pip compile` command. Keep
base-specific declarations in `dockers/docker-base-trixie/requirements.in`.

`MODULE.bazel` records the selected infrastructure, Distroless and FIPS versions.
The root infrastructure selection keeps older consumer declarations from choosing
an obsolete compiler or execution-tool runtime. FIPS owns the GNU tool runtime
files required by SymCrypt and the OpenSSL, OpenSSH, Kerberos and SymCrypt
runtime/debug TAR contracts.

The source-base roots select published
`sonic-build-infra@0.0.22-a12bc826d6f811a157db5c7c02a2fe033896cc14`
and `sonic-fips@0.0.3-0565bc0426dde6a1e7c495de17707c9557f843a9` from
registry `main`. The root retains the targeted
`kernel_runtime_(amd64|arm64)` remote-download policy in
`tools/bazel/common.bazelrc` because dependency `.bazelrc` files are not inherited.

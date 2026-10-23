# Source-built kernel Debian packages

The `@sonic_linux_kernel//:kernel_packages` target builds Linux with
Debian/SONiC's Make packaging recipes in Bazel's `linux-sandbox`. Compilation
runs as the current unprivileged Linux user, without Docker, root or chroot.
The buildimage consumer verifies the registered source and publishes real DEBs
directly into the normal `$(DEBS_PATH)` directory (`target/debs/trixie` by default).
Existing Make dependencies and APT/dpkg installation use those same files.

Supported execution is native Linux AMD64 or ARM64 with user namespaces enabled.
Bazel statically links its compiled execution tools, including gawk and patchelf,
so preparing the declared runtime does not depend on the host libc version.
The declared tool inputs and package contract target Debian Trixie, VS,
unsigned boot image `6.12.41-1`, ABI `6.12.41+deb13-sonic-<architecture>`.
The producer validates its supported settings. The consumer does not maintain a
second support list or silently substitute the Make producer.

## Build with Make

Initialize `src/sonic-linux-kernel` at its recorded gitlink in a clean checkout,
configure VS for the native architecture, then use the shared switch:

```sh
make target/sonic-vs.bin BUILD_WITH_BAZEL_WHEN_AVAILABLE=y
```

With the switch set to `y`, the first kernel, header or downstream module dependency builds
or reuses the registered kernel source and publishes its five DEBs directly.
There is no separate kernel selector or required manual staging step. Unrelated targets do
not prepare the kernel. The default switch value `n` keeps the existing Make
producer. With `y`, unsupported settings and Bazel failures stop the build;
they never fall back to Make.

Make passes its effective architecture, platform, distribution, kernel version
and ABI, cross-build/emulation settings, Debian profiles, signing settings and
external-patch settings to the producer's `//tools/bazel:kernel_config` Bazel
build setting. The producer owns configuration support. Its normalized
configuration is recorded in the action inputs, package manifest and consumer
provenance, so a changed request cannot reuse packages from a different
configuration. Buildimage needs no architecture or version allowlist to select
Bazel when the producer gains another supported configuration.

Choose `y` or `n` for a fresh build directory and keep that choice for subsequent
builds. Switching an existing directory between builders is outside this
interface; use a separate fresh directory for the other builder.

Normal Make selects the maintained registry on `main` and prepares or reuses
the registered source automatically with the shared switch. To prepare the DEBs
explicitly before running Make, use:

```sh
python3 -B tools/bazel/kernel/prepare.py ensure
make target/sonic-vs.bin BUILD_WITH_BAZEL_WHEN_AVAILABLE=y
```

Both the explicit preparation command and normal Make default to registry `main`.
CI uses the same maintained endpoint. A clean normal Make invocation needs only
the shared switch.

The kernel module and gitlink select landed commit
`fbfb9d6bb89705ad2c49fd48cfc608061cf0d83a` from
[kernel #3](https://github.com/securely1g/sonic-linux-kernel/pull/3).
[Registry #124](https://github.com/securely1g/sonic-bazel-registry/pull/124)
registers that landed source version on `main`.

## Explicit source/cache validation

The separate consumer module selects the exact recorded kernel revision and
checks the registered archive identity, complete generated lock and module graph.
For isolated cold/cache verification the lower-level commands remain available:

```sh
python3 -B tools/bazel/kernel/prepare.py plan --state-dir /path/to/new/kernel-state
python3 -B tools/bazel/kernel/prepare.py build --state-dir /path/to/new/kernel-state
python3 -B tools/bazel/kernel/prepare.py verify --debs-directory target/debs/trixie
```

The CLI defaults to the host execution architecture; `--architecture amd64|arm64`
selects an explicit launcher architecture. Repeat `--kernel-config NAME=VALUE`
to pass effective Make settings, for example
`--kernel-config CONFIGURED_PLATFORM=vs`. `CONFIGURED_ARCH` selects the requested
target architecture independently of the native launcher. Unknown or unsupported
settings fail rather than being ignored. Without overrides, the standalone CLI
uses the ordinary native VS defaults.
`plan` does not build. `build` creates a private consumer workspace, output
directory and repository cache with no local disk cache. `--cache-directory PATH`
selects a persistent download/action cache visible to the current process.
`--debs-directory PATH` selects the final package directory; Make passes its
existing `$(DEBS_PATH)`. Package outputs are copied once from Bazel into this
directory, with no intermediate persistent package collection.
`--remote-cache URL`
enables read-only reuse; `--upload-local-results` explicitly permits writes.
`--distdir` accepts exactly the three checksum-pinned Debian source archives.
`--ca-bundle` and `--java-trust-store` supply public trust material. The source
launcher verifies the pinned native Bazel executable and requires the kernel
action to use `linux-sandbox`; it does not start a worker container.

Automatic Make preparation serializes concurrent requests and uses the existing
`SONIC_BAZEL_CACHE_SOURCE` setting. The existing slave mount exposes this cache
at `/bazel_cache`; the kernel uses its `kernel-linux-sandbox/repository_cache` and
`kernel-linux-sandbox/disk_cache` subdirectories as the slave user. This fixed
namespace avoids the old worker's root-owned kernel cache. Kernel preparation runs
directly in that environment and needs no Docker daemon or host-path mirrors.
For a direct host invocation, pass the actual cache path with
`--cache-directory /path/to/cache`. The kernel is prepared only when its
Make dependency is reached.

Verified DEBs in the normal package directory survive unrelated buildimage/SWSS
commits: their kernel, source archive, tool, configuration and tracked handoff-input identities
must still match, and every package digest/control field is rechecked. Original
build provenance and unchanged package/manifest timestamps remain intact. Local
automatic builds retain complete private locks, graphs, plans, invocation and
receipt records under `target/bazel-kernel-state`; successful heavy workspaces
are pruned, while failed workspaces remain for diagnosis.

A changed source/tool/configuration input can rebuild the entire kernel. A
cache hit reuses the source-built package outputs and skips kernel compilation.
This does not claim per-object caching or independent cold byte reproducibility.

## Package contract

The normal DEB directory contains these five kernel DEBs and two metadata
sidecars alongside packages from other producers:

| Output | Purpose |
| --- | --- |
| `linux-image-ABI-unsigned_6.12.41-1_ARCH.deb` | Kernel, modules, configuration, maintainer scripts and dependencies |
| `linux-image-ABI-dbg_6.12.41-1_ARCH.deb` | Matching kernel/module symbols, build-ID lookup and patched source |
| `linux-headers-ABI_6.12.41-1_ARCH.deb` | Architecture headers and generated configuration/Module.symvers |
| `linux-headers-6.12.41+deb13-common-sonic_6.12.41-1_all.deb` | Common headers |
| `linux-kbuild-6.12.41+deb13_6.12.41-1_ARCH.deb` | External-module build tools |
| `kernel-packages.json` | Schema 3 source/archive/tools identity, package checksums/control fields, debug pairing and all created packages |
| `kernel-provenance.json` | Original buildimage commit, handoff-input digest, kernel gitlink, target and verified registry resolution |

`ABI` includes the architecture suffix and `ARCH` is `amd64` or `arm64`.
Verification checks the five manifest-listed packages and their metadata,
rejecting missing outputs, changed digests and mismatched package control fields,
source inventories, architecture, tool identity or resolution. Unrelated DEBs
and files in the shared directory are preserved. The five exports must agree
with the complete created-package inventory. Publication verifies temporary
files before replacing the selected kernel files and preserves unchanged file
timestamps; it never replaces the shared directory.
The original buildimage commit remains recorded even when a later commit with
identical handoff inputs consumes the same packages.

## Make and package installation

The kernel package targets share a lazy preparation prerequisite that publishes
all five DEBs directly to `$(DEBS_PATH)`. Their normal dependency/install
relationships remain available, without a `SONIC_COPY_DEBS` import or extra copy.
Every request revalidates the selected files. Bazel's disk/remote action cache
owns kernel build reuse; selected Bazel packages bypass Make's package cache.
A conditional dependency hook tracks their bytes and manifest for rootfs and
external-module consumers. `Makefile.cache` and the native kernel cache behavior
remain unchanged. There are no kernel builder-switch stamps or forced restores.
Runtime installation excludes the matching debug DEB, which is installed only
when debug tools are requested.

APT installs the image DEB with its actual dependencies and maintainer scripts,
which run depmod, create the initial initramfs and manage boot links. Existing
SONiC image hooks then update the initramfs. Debug images also install the debug
DEB. Headers and kbuild use the standard slave package-install dependencies for
external modules. There is no manual kernel TAR installation or fabricated dpkg
state.

The SBOM verifies the installed dpkg image record and kernel/module/copyright
bytes against the selected DEB. It retains a Debian-package identity, exact
source revision, Debian source ancestry, SONiC patch pedigree and module-to-
kernel ABI dependencies. Other package recipe records remain available.

## Validation and public artifacts

The `Bazel kernel DEBs` matrix runs on native AMD64 and ARM64 runners. Each
builds the exact registered kernel source as an unprivileged user under
`linux-sandbox`, then builds into a fresh Bazel output directory using a local
HTTP remote cache. Verification requires the cold action's sandbox runner and
the second action's remote cache hit, with identical DEB hashes. It also repeats
`ensure` and requests the DEBs twice through the production Make rules, checking
that unchanged package timestamps and unrelated files are preserved.

A separate installation step uses a disposable pinned Trixie container to test
the already-built DEBs. It does not compile the kernel. This checks installed
package status/version/architecture, dependencies, kernel hooks and initramfs,
boot links, GDB source/type/line lookup and an external module compiled and
installed using the shipped headers. Docker and installation privileges belong
to this package acceptance check, not the kernel build or cache-hit proof.

The producer repository independently verifies native builds and cache hits.
Fixture tests here cover provenance, package-control mismatches, corrupt
outputs, concurrent preparation, unchanged commit reuse, repeated Make requests,
fixed builder selection, cache inputs, installed dpkg records and SBOM identity.
Make/Git fixtures cover normal clones, linked worktrees, ordinary slave cache
mounts and lazy preparation without a Docker prerequisite. They use small actual
DEBs but do not compile a kernel.

Build, cache-hit, verification and installation command diagnostics are visible
in the Actions console. Public artifacts contain the five verified DEBs and a
fixed validation summary. Complete generated locks, graphs, handoff manifests
and raw log files are excluded from artifact uploads; no generated lock is
committed. This public artifact is not the complete Make
handoff. The full Make VS workflow requires its existing manual opt-in and also
executes unrelated package producers. These checks do not assemble an installer,
boot a guest, load a module into a running guest or establish forwarding.

## Dependency state

Local builds and CI use the maintained registry endpoint on `main`. The direct
sandbox build selects the landed
[shared tools #62](https://github.com/securely1g/sonic-build-infra/pull/62),
[tools registration #122](https://github.com/securely1g/sonic-bazel-registry/pull/122),
[kernel source #3](https://github.com/securely1g/sonic-linux-kernel/pull/3) and
[kernel registration #124](https://github.com/securely1g/sonic-bazel-registry/pull/124).
Both source and tools revisions are fixed in `MODULE.bazel`, and the kernel
source revision must match the buildimage gitlink. Native AMD64 and ARM64
source/cache validation runs on Ubuntu 22.04 with unprivileged user namespaces;
installed-package validation runs separately in Debian Trixie.

# Registry kernel consumption

The root `MODULE.bazel` selects `sonic-linux-kernel` from the maintained Registry.
Build the registered source directly from the buildimage workspace:

```sh
bazel build @sonic_linux_kernel//:kernel_packages \
  @sonic_linux_kernel//:linux-image.runtime \
  @sonic_linux_kernel//:linux-image.runtime_metadata \
  @sonic_linux_kernel//:linux-image.control \
  @sonic_linux_kernel//:boot @sonic_linux_kernel//:kernel_metadata.json
```

The Trixie target platform selects the kernel architecture; the execution
platform follows the build host. To cross-compile ARM64 on AMD64:

```sh
bazel build --platforms=@sonic_build_infra//platforms:aarch64_trixie \
  --host_platform=@sonic_build_infra//platforms:x86_64_trixie \
  @sonic_linux_kernel//:kernel_packages
```

The producer owns supported kernel configurations. The `kernel_config` build
setting carries Make variables when building through the Make bridge.

## Consumers

- Host rootfs consumes `linux-image.runtime` and `linux-image.control`.
  `linux-image.runtime_metadata` provides the shared `RuntimePackagesInfo` interface.
- Normal and kdump initramfs consume the runtime TAR and `kernel_metadata.json`.
  The image owns userland, hooks, configuration and configuration-dependent depmod.
- ONIE assembly consumes the `boot` directory and the same kernel identity.
- External modules load `kernel_module` from the producer's
  `//tools/bazel:kernel.bzl` and depend on its `kernel_packages` provider.
- Make installs the five maintained Debian packages from `kernel_packages`.

Image assembly can reference these public producer outputs directly to obtain
the runtime TAR and boot files before Debian package compression.

## Make package publication

```sh
make target/sonic-vs.bin BUILD_WITH_BAZEL_WHEN_AVAILABLE=y
```

The lazy kernel prerequisite invokes `packages.py build`. In dedicated VS CI it
first tries the verified remote restore described below. On a miss, it runs
`bazel build` in the root workspace, obtains the declared package paths with
`cquery`, validates all package digests, and publishes them into the existing
`DEBS_PATH`. Concurrent Make requests share the package publication lock. The
publication record binds the package manifest to the root kernel module version
and original consuming buildimage revision.

```sh
python3 -B tools/bazel/kernel/packages.py build --architecture amd64
python3 -B tools/bazel/kernel/packages.py verify --architecture amd64
```

Make passes its effective kernel configuration and existing cache settings.
Package installation, initramfs hooks and external-module installation retain
their existing Debian package contracts. The verification compatibility entry
in `prepare.py` accepts older retained package provenance as well as direct
root-workspace publication records.

## CI

`bazel-kernel-bundle.yml` builds the public Registry outputs in the root module,
verifies publication and repeated Make consumption, and installs the built DEBs
in a disposable Trixie system. Its optional fresh-output cache check requires
a remote hit of the kernel action. Uploaded artifacts contain the five checked
DEBs and the public validation summary.

Registry CI covers the producer's AMD64 native and ARM64 cross targets. The
buildimage workflow checks direct installation outputs and Debian package handoff.

### Restore kernel DEBs in a fresh installer job

The dedicated native AMD64 Trixie VS installer enables
`SONIC_VS_KERNEL_REMOTE_REUSE=1` and supplies the remote cache endpoint. An
eligible master build publishes a small index in the existing workflow artifact
and stores the five DEBs, manifest, original provenance and compressed root
lockfile in the Bazel content-addressable store. The first master build for a
new request runs Bazel to populate the index.

A fresh job reads that trusted master index and validates the local request and
resolved inputs before downloading any DEB. The request covers the unsigned VS
configuration, native builder and Bazel executable, root module declarations,
local overrides and consumer inputs. The resolution check covers the pinned
transitive registry inputs, including previously missing lookups, extension
records, yanked versions and release checksums. A root lock already populated
by another target can be used when its kernel records agree.

Before installing the five DEBs, the job verifies their sizes, SHA-256 digests,
Debian control fields and the manifest and provenance against the Make request.
A verified hit skips both `bazel build` and `bazel cquery`. Changed or
unverifiable inputs, an incompatible index or an evicted blob trigger the
ordinary root Bazel build. Other architectures and configurations also take
that path. Legacy launcher indexes use a separate schema.

When the root declarations or transitive resolution change, review the inputs
in `root_request.py` and `root_resolution.py` against a fresh kernel-only
analysis before enabling direct restore for that graph. The latter script takes
the analysis lockfile as an argument and emits the registry membership and
fingerprints for review.

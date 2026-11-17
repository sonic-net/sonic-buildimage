# Sysmgr OCI images with Bazel

The AMD64/Trixie image consumes the Sysmgr owner's runtime TAR and matching
symbols, together with the Trixie Protobuf owner's runtime TAR and metadata.
The source-built `docker-config-engine-trixie` parent supplies Common and the
shared Python/configuration runtime. Sysmgr compilation and generator tests
remain in `src/sonic-sysmgr`; this image does not compile a second copy or build
DEB packages.

`../BUILD.bazel` composes the runtime and debug OCI layouts with `rules_oci`,
declared TAR inputs and the locked Debian runtime dependencies. The debug image
extends the exact runtime layers, adds matching source symbols and the existing
debug-tool closure, and retains the original runtime ELF files. Existing shared
archive rules expose `docker-sysmgr.gz` and `docker-sysmgr-dbg.gz` with their
original image tags for Make consumers.

The Dockerfile's Supervisor configuration, critical-process list, rsyslog
identity and config-engine status marker are preserved. Package-state metadata
comes from the source owners and the selected Debian inputs. Make's existing
prerequisites and mounts remain in place; Bazel selection applies only to the
supported AMD64/Trixie VS/FIPS configuration with ASAN disabled. Other profiles
retain the existing path.

## Build and validate

Use a clean checkout and native AMD64/Trixie execution environment. Initialize
the pinned gNOI source, then run the build driver with the complete review
catalog. It prepares the original Make manifest labels, audits the configured
graph for DEB production, and builds the archives, layouts and evidence.

```sh
git submodule update --init -- src/sonic-sysmgr/gnoi
python3 -B tools/bazel/ci/sysmgr_image.py build \
  --artifacts artifacts/sysmgr
```

Run validation separately in the same environment and output base:

```sh
python3 -B tools/bazel/ci/sysmgr_image.py validate --artifacts artifacts/sysmgr
python3 -B tools/bazel/ci/sysmgr_image.py incremental \
  --artifacts artifacts/sysmgr
```

Static validation checks installed source/configuration bytes, ownership,
permissions, inherited files and accounts, dependency metadata, AMD64 ELF and
SONAME links, matching debug symbols, manifest labels and archive tags. The
incremental check measures a cached no-op, edits the actual Supervisor input,
checks that both images change while their source owners and parent stay
unchanged, and restores the input and original image/archive bytes.

For installed-service validation, use the same source revision and copied
`artifacts/sysmgr` directory on an AMD64 host with Docker:

```sh
python3 -B tools/bazel/ci/sysmgr_loaded.py --artifacts artifacts/sysmgr
```

This loads both archives, checks their actual configuration/layer digests, and
starts the original installed Supervisor and Sysmgr daemon. Private Redis and
a rejecting mock D-Bus service exercise real request/status and error handling
without invoking a host reboot. The fixture runs with no network, a read-only
root filesystem and no Linux capabilities. Debug validation also checks source
lookup with the installed symbols. The separate CI runtime job performs this
check against the uploaded archives, including their recorded SHA-256 hashes.

These image checks do not establish full `sonic-vs.bin` acceptance. That remains
a separate image-integration check.

## Updating inputs

Keep new Sysmgr C++ sources, generated interfaces and reusable runtime/debug
outputs in the source owner's BUILD files and tests. Add image-only files to
the Dockerfile-aligned mapping in `../BUILD.bazel`, and update the corresponding
installed-content expectations in `tools/bazel/ci/sysmgr_image.py`. CI compares
the installed configuration with the original files and generated text.

Sysmgr uses `@sonic_build_infra//apt:apt.lock.json` for its runtime, debug and
test package sets. Refresh those sets through `sonic-build-infra/apt/requests.json`
and the infrastructure owner's APT workflow. Preserve the snapshot URLs and
review the shared lock, selected runtime closure and package metadata together.
Keep the complete review catalog pinned until the source and registry
prerequisites land; do not substitute local sibling producer overrides.

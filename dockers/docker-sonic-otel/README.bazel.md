# Source-built OTEL container

The opt-in AMD64/Trixie VS path assembles `docker-sonic-otel.gz` and
`docker-sonic-otel-dbg.gz` directly from source-owned runtime and debug TARs.
The collector is `otelcol-contrib@0.144.0.sonic.1`; its registration is
[registry PR #196](https://github.com/securely1g/sonic-bazel-registry/pull/196).
The source owner retains the original 0.144.0 component manifest, Go module
versions, installed configuration and unit, and matching detached symbols.
This image defines no Debian package producer.

The runtime extends the source-built `docker-config-engine-trixie` image.
Its inherited project closure includes Common, libnl, config-engine Python
packages, event services and the Rust supervisor listener, together with the
base's socat, iproute2, OpenSSL, Kerberos and SymCrypt producers. Pinned Debian
packages supply external runtime dependencies. The original Dockerfile's
`ca-certificates`, `curl` and `wget` selection uses the shared APT layer rule;
it verifies compatibility against the actual parent before adding files.

The image keeps the original `start.sh`, `otel.sh`, supervisor configuration,
critical-process declaration and Make manifest. The shared environment input
preserves `IMAGE_VERSION` and `DEBIAN_FRONTEND`. The official package's service
account is represented explicitly as UID/GID 999, home `/home/otelcol-contrib`,
`/sbin/nologin`, and no created home directory; the account rule checks the
actual parent's identities. The public Distroless rule locks the account
with `!` directly in passwd/group; the original `useradd` uses `x` there and
locks separate shadow entries. This preserves the locked service-account
behavior and inherited shadow files, not byte-for-byte password database
representation. `/usr/local/bin/otelcol-contrib` points to the
owner's `/usr/bin/otelcol-contrib`. The shared normalization rule moves the
owner's `/lib/systemd/system` unit into the merged-usr parent's `/usr/lib`.
The original post-install script enables the unit only when `systemctl`
exists. Validation checks that this container base has no `systemctl` and
that no enablement link is invented.

The debug image extends the exact runtime layers. It adds the existing debug
tools, all inherited source symbols, the collector's matching symbols, and
the Make debug contract's FIPS OpenSSH client, `_ssh` group and matching
symbols. It preserves every runtime ELF byte. Existing non-AMD64, non-Trixie,
non-VS, non-FIPS, ASAN, cross-build and renamed debug profiles keep their
legacy Make producer and prerequisites.

## Build and check

Use the maintained SONiC registry and BCR from native AMD64 Debian Trixie.

```sh
python3 tools/bazel/ci/otel_image.py build \
  --artifacts artifacts/otel
python3 tools/bazel/ci/otel_image.py validate --artifacts artifacts/otel
python3 tools/bazel/ci/otel_image.py incremental \
  --artifacts artifacts/otel
```

On a Docker-capable AMD64 worker, run the separately loaded-image checks:

```sh
python3 tools/bazel/ci/otel_loaded.py --artifacts artifacts/otel
```

The build phase prepares Make metadata and its environment input, audits the
configured source graph for package production, and retains source, image and
archive hashes. Validation checks exact parent preservation, normalized
collector files, package metadata, account state, archive tags, platform,
labels, runtime/debug ancestry and every source-owned ELF/debug pair. The
incremental phase edits and restores the original launcher, checking both
images/archives change while the collector and parent stay unchanged.

Installed validation runs the original `otel.sh` with missing, malformed and
invalid collector configurations, then submits a real private-loopback OTLP
trace to the actual installed collector and checks its file export. It also
runs the unmodified supervisor/startup chain with a declared private Redis
fixture, requires both event listeners and the collector to stay running,
and checks the startup owner/version state. The fixture never enters either
production image. The debug check proves source/line/type lookup succeeds
only with matching installed symbols. These tests do not claim production
telemetry backend, host namespace, hardware or VS boot coverage.

CI extends the existing `bazel-oci.yml` native job and uses a dependent job for
loaded images. It retains the selected source owner's three contract tests,
public source/image receipts, debug checks and incremental results.

The shared infrastructure APT lock supplies the `otel_debian` runtime collection,
plus the matching existing debug and Redis test collections used by FRR.

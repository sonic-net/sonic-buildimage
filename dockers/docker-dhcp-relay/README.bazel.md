# DHCP relay images with Bazel

The native AMD64 Debian Trixie path builds the runtime and debug images from
source-owned installation TARs and a source-built Python wheel. It does not
produce Debian packages. The remaining distribution libraries come from the
shared, checksum-locked APT inputs in `sonic-build-infra`.

| Installed component | Producer |
| --- | --- |
| ISC DHCP relay | `@isc_dhcp//:isc-dhcp-relay.runtime` |
| Native DHCPv4 relay | `@sonic_dhcp_relay//dhcp4relay:runtime` |
| DHCPv6 relay | `@sonic_dhcp_relay//dhcp6relay:runtime` |
| DHCP monitor | `@sonic_dhcpmon//:runtime` |
| DHCP utilities | `//src/sonic-dhcp-utilities:sonic_dhcp_utilities_wheel` |
| Common, config engine and supervisors | `//dockers/docker-config-engine-trixie:docker-config-engine-trixie` |

The Python runtime installs the utility wheel through the shared wheel installer.
Its dependency plan reads the wheel's actual metadata. The checked runtime TAR
binds this plan to the committed Python lock and verifies the dependencies already
installed by config engine. The debug image extends the exact runtime image and
adds matching source symbols and debugging tools.

## Build and validate

On normal CI, **Validate docker-dhcp-relay (AMD64)** downloads the DHCP relay
archives and owner records from the VS installer build. It verifies installed
files and exercises DHCPv4, DHCPv6 and the monitor against a private Redis
instance and virtual links. Enabling the VS debug layer also checks the debug
archive, ELF build IDs and source lookups.

Use a native AMD64 Trixie worker with the repository's declared toolchains and
dependencies available. From a clean checkout:

```sh
python3 -B tools/bazel/ci/dhcp_relay_image.py build --artifacts artifacts/dhcp-relay
python3 -B tools/bazel/ci/dhcp_relay_image.py validate --artifacts artifacts/dhcp-relay
python3 -B tools/bazel/ci/dhcp_relay_image.py incremental --artifacts artifacts/dhcp-relay
```

The build audits the complete selected Bazel action graph before execution and
rejects DEB outputs or package-building commands. It prepares image metadata with
the existing metadata-only Make entry, builds the source layers, and writes
`docker-dhcp-relay.gz` and `docker-dhcp-relay-dbg.gz`. Validation checks the ordered
image contents, parent ancestry, installed package metadata, Python inheritance,
and runtime/debug symbol correspondence. The incremental check changes a template,
checks which outputs rebuild, then restores it and verifies the original digests.

On an AMD64 worker with Docker and the required isolated network capabilities:

```sh
python3 -B tools/bazel/ci/dhcp_relay_loaded.py --artifacts artifacts/dhcp-relay
```

The installed-runtime check loads temporary image tags and starts the original
entry point inside isolated containers. It checks template profiles, daemon
startup, DHCP utilities and debug symbol lookup using a private Redis fixture.
It does not change host networking or use an external DHCP server.

The `Bazel OCI` workflow's `source_image=dhcp-relay` dispatch runs the DHCP utility
tests and original CLI plugin suite, both image builds, content/incremental checks,
and installed-runtime checks.
Set `source_revision` to the full reviewed commit. The K3s build uses the remote
Bazel cache; it does not require a persistent local cache. This dedicated dispatch
does not select the full VS installer or other container builds.

## Review prerequisites and limits

The review branch consumes the merged [relay source PR #5](https://github.com/securely1g/sonic-dhcp-relay/pull/5)
through the merged [registry PR #235](https://github.com/securely1g/sonic-bazel-registry/pull/235).
The shared wheel metadata adapter from merged
[Infra PR #127](https://github.com/securely1g/sonic-build-infra/pull/127)
is published by [registry PR #444](https://github.com/securely1g/sonic-bazel-registry/pull/444).
The integration consumes `sonic-build-infra@0.0.40-6c9cd9477816ad129bf8d4501c9259504f3f85ca`
from the maintained registry, using the same registry selection in ordinary and
dedicated DHCP workflows.

The existing Make build remains the fallback for unsupported profiles. A fresh
Make reference build can generate DEBs and is outside this task's validation
scope. Runtime and debug Make equivalence therefore requires retained, compatible
reference archives with verified source/configuration provenance. Successful
source tests or OCI construction alone do not establish that equivalence.

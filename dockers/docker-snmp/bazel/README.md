# Source-built SNMP image (AMD64)

This adds the native AMD64 Trixie/FIPS VS profile to the shared Bazel image
selector. It consumes the existing source-built config-engine image directly,
`sonic-platform-common`'s wheel, and `sonic-snmpagent`'s wheel and install TAR.
It does not build Debian packages or import Make-produced project artifacts.

| Input | Existing Make path | Bazel path |
| --- | --- | --- |
| Platform Python APIs | `SONIC_PLATFORM_COMMON_PY3` | `@sonic_platform_common//:sonic_platform_common_wheel` |
| AgentX agent | `ASYNCSNMP_PY3` | `@sonic_snmpagent//:sonic_snmpagent_wheel` |
| `sysDescr_pass.py` | `python3 -m sonic_ax_impl install` | `@sonic_snmpagent//:snmp_install` |
| Common, py-common and config-engine | Inherited config-engine container | Existing public config-engine OCI target and its source dependency graph |
| External SNMP tools | Trixie APT repositories | SNMP runtime set in the shared Trixie APT lock |
| Third-party Python modules | Make’s per-image version list and inherited base wheels | Hashed lock compiled from the same list for hiredis, psutil, python-arptable and smbus. YAML and natsort come from config-engine |
| Debug OpenSSH client | `FIPS_OPENSSH_CLIENT` | Existing `@sonic_fips//openssh:openssh_client_pkg`, with its matching symbols |
| Final output | Docker archive | Reproducible `docker-snmp.gz` and `docker-snmp-dbg.gz`, exported through the shared archive rule |

Regenerate the external Python wheel lock with uv 0.12.18 from the repository root:

```sh
uv --system-certs pip compile files/build/versions-public/dockers/docker-snmp/versions-py3 \
    --python-version 3.13 --generate-hashes --no-emit-index-url \
    --output-file dockers/docker-snmp/requirements_lock.txt
```

The installed-image probes compare the selected Python distribution versions with Make’s
SNMP and base version lists in both archive variants.

The wheel producers are in their owning repositories. Platform-common remains a
separate runtime dependency of this Docker; the current SNMP source does not
import it or need it to build its wheel. The complete image still provides both.
No source is copied into this repository to rebuild either wheel.

The public image targets are `//dockers/docker-snmp:docker-snmp` and
`:docker-snmp-dbg`. Their `.gz` targets supply the existing installer archive
contract. `:all_debug_symbols` contains matching symbols for the inherited
project ELF files and the debug-only FIPS OpenSSH client. The two Python owners
produce pure Python wheels and have no ELF debug package.

## Build and validate

```sh
make -f tools/bazel/prepare_manifests.mk \
  'MANIFEST_METADATA=rules/docker-base-trixie.mk rules/docker-config-engine-trixie.mk rules/docker-snmp.mk' \
  BUILD_WITH_BAZEL_WHEN_AVAILABLE=y BLDENV=trixie CONFIGURED_PLATFORM=vs \
  CONFIGURED_ARCH=amd64 INCLUDE_FIPS=y ENABLE_ASAN=n DBG_IMAGE_MARK=dbg DOCKERS_PATH=dockers
bazel build //dockers/docker-snmp:docker-snmp.gz //dockers/docker-snmp:docker-snmp-dbg.gz
```

Make supplies `SONIC_IMAGE_VERSION` through the declared `:image_version` build
setting. Direct callers can set `--//dockers/docker-snmp:image_version=VERSION`.
A changed version invalidates the OCI configuration, without rebuilding wheels.
Make-generated labels and normal/debug tags remain intact. The legacy Dockerfile
places `EXPOSE 161/udp 162/udp` only in its builder stage; its final `FROM $BASE`
inherits the base metadata. This migration preserves that actual final-stage
behavior rather than adding new published ports.

The AMD64 archive job in `.github/workflows/bazel-oci.yml` builds both SNMP variants and checks
installed wheel bytes, startup scripts and template inputs, AMD64 ELF linkage,
account and directory ownership, imported and source package metadata, matching
source debug symbols, preserved runtime ELF bytes in debug, and template
edit/restore invalidation with identical restored image digests. A later job
loads those exact archives and runs real SNMP GET/GETNEXT against the installed
SysName MIB through Common, private Redis, AgentX and snmpd. It also renders the
normal/chassis supervisor templates and service configuration. The debug probe checks
source-line lookup for the packaged `ssh` and `sonic-db-cli` symbols.

The Redis fixture is a separate test-only TAR. It is mounted by CI and is never
an image layer. Both installed-image probes run as the declared unprivileged
SNMP account with no network access outside the container and no source checkout
mounted into the Python import path. Hardware-specific and multi-namespace MIB
behavior is not established by this fixture.

The full VS installer uses the repository's standard OCI workflow. Its result is
tracked separately from the archive and installed SNMP checks.

## Imported package installation behavior

The reviewed snmpd post-install script creates `Debian-snmp`, uses `/bin/false`,
sets `/var/lib/snmp` ownership and changes `/etc/snmp/snmpd.conf`'s group.
The image declares these effects without running maintainer scripts. Stable
UID101/GID102 are checked against the inherited account files; an ID collision
fails assembly. The default config retains mode0640 and gets GID102. The SONiC
startup script still renders the final service config at runtime.

Runtime package records come from the actual selected controls. Debug adds its
own selected controls and the FIPS owner's source-package identity. Wheels keep
their real distribution metadata rather than invented Debian records. The debug
client retains the owner's `_ssh` GID104 and setgid mode; Vim alternatives use
the existing shared implementation. Services are managed by SONiC supervisor,
so the build does not enable or start Debian init/systemd services.

Both source owners are published on the registry's maintained `main` branch.
SNMP selects `2.1.0-a27e093fac394f7871ec1b19bb12ca6739c29fa9`, published by
[#164](https://github.com/securely1g/sonic-bazel-registry/pull/164), and
platform-common selects `1.0.0`. Their source commits and archive integrity
remain pinned by the registry entries; neither needs a source archive override.

To reproduce the build and run the Docker probe on native AMD64 Trixie:

```sh
git submodule update --init -- src/sonic-sysmgr/gnoi
python3 -B tools/bazel/ci/snmp_image.py build --artifacts artifacts/snmp
python3 -B tools/bazel/ci/snmp_image.py validate --artifacts artifacts/snmp
python3 -B tools/bazel/ci/snmp_image.py incremental --artifacts artifacts/snmp
python3 -B tools/bazel/ci/snmp_loaded.py --artifacts artifacts/snmp
```

CI retains the generated module lock, resolved graph, receipt and validation
reports with the archives. The SNMP runtime, debug tools and test Redis package
sets are maintained in the shared `sonic-build-infra` Trixie APT lock.

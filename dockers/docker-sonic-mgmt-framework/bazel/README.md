# Management-framework image

This path builds the AMD64/Trixie VS management service from source-owned TARs.
It uses the source-built config-engine parent, management-common's models and
schema, and management-framework's REST server, generated UI and patched klish
CLI. The matching owner debug TAR is included in the debug image. The existing
Make package producer remains available for other configurations.

| Input | Owner and public target |
| --- | --- |
| Base libraries, Common Python binding, cfggen and SONiC Python packages | `//dockers/docker-config-engine-trixie:docker-config-engine-trixie` |
| CVL schema and YANG data | `@sonic_mgmt_common//dist:runtime` |
| REST, UI and CLI | `@sonic_mgmt_framework//dist:runtime` |
| REST/CLI symbols | `@sonic_mgmt_framework//dist:runtime.debug_symbols` |
| Debug OpenSSH client | `@sonic_fips//openssh:openssh_client_pkg` |
| Requests and its dependencies | Framework's canonical `requirements_lock.txt`, imported through upstream pip rules |
| Unmodified Debian libraries and debug tools | The shared build-infra Trixie APT lock selected by the image root |
| Startup scripts, supervisor settings and management template | This Docker directory, copied with their existing paths and modes |

The shared APT selector rejects native ELF conflicts with the parent and retains
the declared package overrides for other files. Real imported control
records update its package inventory. Common and Framework TARs are consumed
directly; this does not invent a dpkg installation for either source payload.
The debug image extends the exact runtime image and preserves its native files.
No target or validation step generates a DEB.

The Make selector preserves the `mgmt-framework` container name, mounts,
`INCLUDE_MGMT_FRAMEWORK` behavior, and host `/usr/bin/sonic-cli` wrapper. Its
manifest labels still come from Make's existing generator. The native selector
requires VS, AMD64, Trixie, FIPS, ordinary execution without ASAN, and the standard
`dbg` suffix. Other profiles use their original Make prerequisites.

The image consumes the root registry configuration and the reviewed Trixie lock
from `sonic-build-infra`. The runtime selects its four Debian roots from
`framework_cli_target_debian`. The debug image uses `sysmgr_debug_debian`,
and the installed-service fixture uses the shared test Redis package.

On a native AMD64 Trixie worker, with the Framework registration available
to the root module:

```sh
BAZEL=bazel
python3 -B tools/bazel/ci/mgmt_framework_image.py build --bazel "$BAZEL" --artifacts artifacts/mgmt-framework
python3 -B tools/bazel/ci/mgmt_framework_image.py validate --artifacts artifacts/mgmt-framework
python3 -B tools/bazel/ci/mgmt_framework_image.py incremental --bazel "$BAZEL" --artifacts artifacts/mgmt-framework
python3 -B tools/bazel/ci/mgmt_framework_loaded.py --artifacts artifacts/mgmt-framework
```

The build phase prepares metadata and audits the selected action graph before
building. Post-build checks verify the original owner bytes and metadata,
inherited files, ELF/debug pairs, OCI structure, archive tags and deterministic
gzip output. A no-op and management-template edit/restore must preserve source
TARs and reproduce both original image/archive digests after restoration.

Installed checks load only those verified archives under temporary tags. Each
container preserves its declared service user with a read-only root, private
writable service directories, no capabilities and no external network. A private
Redis fixture supplies both the Unix socket and loopback TCP endpoint; both must
identify the actual child process. The original Supervisor starts the installed
start hook, rsyslogd and REST launcher. Its service states, process ancestry,
configuration status and persistent process identities are checked.
The test verifies native library loading, Python imports,
cfggen template rendering, certificate generation, verified REST
TLS, RPC registration, the installed UI and command-tree parsing through the
installed CLI launcher. Both valid and invalid commands are checked with klish's
syntax-only option. The Redis fixture is test-only and is not installed in either
image.

These fixtures disable REST authentication; host PAM/TACACS, hardware behavior
and full VS installer/boot acceptance are separate. They do not change the
production startup script's default authentication. Its deprecated `-ui
/rest_ui` argument is preserved; the current server uses its default installed
UI directory `/etc/rest_server/ui`.

The shared Go graph uses one root gNMI work file, Go 1.25.9, modern glog 1.2.4,
and the existing SONiC-patched public goyang and ygot modules. Framework joins
through its go.mod requirements. The root explicitly preserves x/text 0.39.0
from the original owners while combining their compatible net/crypto/sys
requirements. Management-common's cumulative source includes deterministic
model discovery. These are changed dependency inputs: the earlier Framework
image acceptance does not validate this graph. Current ordinary owner and
installed image acceptance remain separate gates.

Framework and Common own their reusable source and generator definitions.
Update the selected Debian roots in the image module when its dependencies
change. New Debian versions and package records belong in the shared Trixie
APT lock.

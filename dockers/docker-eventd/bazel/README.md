# Eventd image

This path builds native AMD64/Trixie Eventd runtime and debug images for the
FIPS-enabled VS configuration. Its config-engine parent already builds eventd,
eventdb, events_tool, Common and their native/Python dependencies from source.
The child adds the existing service scripts and five generated rsyslog configs.
It consumes owner TARs and imports locked Debian packages; it generates no DEBs.

| Input | Owner and target |
| --- | --- |
| Base libraries, event services, Common, Python and YANG files | `//dockers/docker-config-engine-trixie:docker-config-engine-trixie` |
| Event service runtime and symbols | `//src/sonic-eventd:eventd_runtime` and `:eventd_runtime.debug_symbols`, inherited through the parent |
| Service scripts and supervisor configuration | `//dockers/docker-eventd/config:files` |
| Five host rsyslog configs | Original JSON files and `//files/build_templates:rsyslog_plugin.conf.j2`, rendered by the locked upstream `jinjanator` entry point |
| Image version | Make's `SONIC_IMAGE_VERSION`, prepared as the declared `target/bazel-manifests/docker-eventd/image.env` |
| Debug OpenSSH client | `@sonic_fips//openssh:openssh_client_pkg` and matching owner symbols |
| Debug tools | `sysmgr_debug_debian` from the shared build-infra Trixie APT lock |
| Redis server for installed-service tests | `@eventd_deps//redis-server`, packaged only in the test fixture |

SONiC's existing `docker_image_ctl.j2` copies the generated rsyslog configs from
this container to the host. Preserve their names, contents and location under
`/etc/rsyslog.d/rsyslog_plugin_conf`. The referenced host rsyslog plugin is not an
installed dependency of this container in the Make Dockerfile. The child also
retains Make's container name, mounts, include flags and builder prerequisites.
Unsupported architectures, platforms, distributions, cross-builds, ASAN and
nonstandard debug suffixes retain the existing Make producer.
Version preparation rejects empty/multiline values and dollar signs, since
`rules_oci` would expand dollar references against the parent environment.
Rejection preserves the previous declared input.

The shared Bazel OCI workflow builds both archives on its native AMD64 Debian
Trixie runner and uploads them in the `eventd-image-amd64` artifact. The
installed-service job downloads those archives and checks them with Docker.
To run the same checks from a checkout, run the first three commands on native
AMD64 Trixie and the last on an AMD64 Docker host with that checkout and the
same artifact directory.

```sh
python3 -B tools/bazel/ci/eventd_image.py build --artifacts artifacts/eventd
python3 -B tools/bazel/ci/eventd_image.py validate --artifacts artifacts/eventd
python3 -B tools/bazel/ci/eventd_image.py incremental --artifacts artifacts/eventd
python3 -B tools/bazel/ci/eventd_loaded.py --artifacts artifacts/eventd
```

The build phase prepares metadata, audits the action graph, and builds explicit
TAR, OCI and Docker archive targets. Separate post-build checks compare inherited
files, source bytes, architecture, package state, debug symbols and Make labels.
Editing `dockers/docker-eventd/host_events_info.json` changes both child
images while the config-engine parent keeps its digest. A no-op build and
restoring the original JSON must reproduce the accepted digests.

Installed checks use temporary Docker tags, a read-only root, no capabilities,
no external network and the inherited unprivileged Redis account. A private
Redis process and container-local ZMQ endpoints exercise the installed eventd,
eventdb wrapper and events_tool. Two distinct published messages must appear in
EVENT_DB. The empty-profile skip and nonempty-profile startup are both checked.
Only the fixture's Redis configuration and event profile use writable mounts;
the production archive is unchanged. The installed `j2` command must reproduce
all five shipped configs and reject missing required template data.
The debug container's installed GDB must resolve source/line/type information
for eventd, eventdb and events_tool, and fail that lookup without their symbols.

The original 25 C++ cases remain covered by the existing event-services source
workflow, including private Redis and rsyslog-plugin fixtures. Full VS boot and
the host's rsyslog/systemd integration remain separate acceptance scopes.

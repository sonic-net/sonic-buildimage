# Native AMD64 gNMI images

The runtime image consumes the public `sonic-gnmi` and `sonic-mgmt-common`
source TARs on top of the source-built Trixie config-engine image. gNMI owns
telemetry, dialout, the gNMI/gNOI clients and their matching detached symbols.
Management Common owns its Go interfaces and generated schemas. Its public
`openconfig_goyang` and `openconfig_ygot` dependencies also supply the gNMI
consumer, so this root does not apply a second copy of those patches. Common owns
the reusable CGo/C++ bindings; the inherited config-engine layer supplies its
Python/native runtime, libyang and libnl libraries.

The runtime APT lock supplies the remaining Debian PAM/Python dependencies.
Debug images add the declared Debian tools, the source OpenSSH client, and the
matching symbols from every source owner. `rules_distroless`, `tar.bzl` and
`rules_oci` handle the package data and image layers directly. No Bazel DEB
producer is selected. Reading pinned Debian inputs is separate from generating
new DEBs.

## Make profile

`rules/docker-gnmi.mk` selects these targets only for native AMD64 VS/Trixie,
FIPS enabled, both gNMI write features enabled, the usual `dbg` image suffix,
no ASAN/cross-build/QEMU, and no custom gNMI Dockerfile extension. Other profiles
continue through their existing Make recipes. `ENABLE_DIALOUT=n` omits the
owner's dialout executable and the original dialout launcher/supervisor block;
the renderer is checked byte-for-byte against the Dockerfile's existing sed
operation. `INCLUDE_SYSTEM_GNMI` still controls installation in the VS image.

Make prepares the original manifest labels and the actual `SONIC_IMAGE_VERSION`
for OCI `IMAGE_VERSION`. The unchanged startup scripts, supervisor dependencies,
critical process declaration and rsyslog configuration are packaged with their
original permissions. The telemetry watchdog and other Docker families remain
outside this image integration.

## Acceptance

The gNMI image builds use the canonical `.bazelrc` registry configuration.
Build, artifact validation and installed-service validation are separate commands;
run them in a clean, committed source checkout:

```sh
python3 -B tools/bazel/ci/gnmi_image.py build \
  --bazel bazel --dialout enabled \
  --artifacts artifacts/gnmi/enabled
python3 -B tools/bazel/ci/gnmi_image.py validate \
  --artifacts artifacts/gnmi/enabled
python3 -B tools/bazel/ci/gnmi_image.py incremental \
  --bazel bazel --artifacts artifacts/gnmi/enabled
python3 -B tools/bazel/ci/gnmi_loaded.py --artifacts artifacts/gnmi/enabled
```

Repeat with `--dialout disabled` and a separate artifact directory. The build
requires native AMD64 Debian Trixie; installed-image validation uses Docker on
a native AMD64 host. Both archive variants preserve the image tags. Validation
checks source bytes, inherited payloads, metadata, architecture, every gNMI
split-symbol pair, and no-op/edit/restore hashes before Docker load.

Loaded-image checks use the actual unchanged supervisor and launcher chain,
private Redis/CONFIG_DB data, two real changing Get results, invalid-database
rejection, both compiled write defaults, loader resolution, and positive/negative
GDB symbol lookup. They verify `container_startup` state and image version too.
Fixtures have no network access or host namespaces. They do not claim hardware,
PAM/TACACS authentication, production credentials, privileged gNOI operations,
telemetry-watchdog integration or VS boot coverage.

The workflow's acceptance job requires both source image construction and the
installed-service checks for enabled and disabled dialout. The source owner and
registry PRs independently validate the original Go suites and ordinary external
consumers. This README describes the intended checks; successful receipts and
current CI results are the acceptance evidence.

# Router Advertiser images

The native AMD64/Trixie VS path reuses the source-built
`//dockers/docker-config-engine-trixie:docker-config-engine-trixie` parent.
Its Common library, Python bindings, configuration engine and supervisor
remain source-owned. This child adds the pinned Debian radvd package and the
unchanged router-advertiser scripts/templates. No project DEBs are generated.

`../BUILD.bazel` composes the runtime image with public `rules_oci` rules and
the existing archive helper. `../config/BUILD.bazel` declares the original
files and radvd account, rather than running Debian maintainer scripts.
The selected package is radvd `1:2.20-1+b1`; its postinst creates a system
user without a private group or home directory. The selected parent leaves
UID 101 free, so this image declares `radvd` as 101:65534 with home
`/run/radvd` and shell `/usr/sbin/nologin`.

The debug image extends the exact runtime image with all matching project
source symbols, source-built FIPS OpenSSH and locked Debian debug tools.
The Debian radvd binary remains an external input; project source symbols
are checked separately from it. Imported control records extend the parent's
package inventory without inventing project package identities.

Make preserves the existing image names, service ordering, installation flag
and runtime mounts. Its supported Bazel selection requires native AMD64 VS,
Trixie, FIPS enabled, no ASAN/cross/QEMU build and the standard debug suffix.
Other profiles retain the existing Make path. The declared `image.env`
retains the exact Make `SONIC_IMAGE_VERSION`; empty, multiline and dollar
values are rejected before replacing an existing input.

Build and validate separately on native Debian Trixie:

```sh
python3 tools/bazel/ci/radv_image.py build --artifacts artifacts/radv
python3 tools/bazel/ci/radv_image.py validate --artifacts artifacts/radv
python3 tools/bazel/ci/radv_image.py incremental --artifacts artifacts/radv
```

The build phase audits the selected action graph before execution and rejects
DEB-producing outputs. The image checks verify source files, inherited bytes,
permissions, package records, platform, tags and detached symbols. The
incremental check edits `radvd.conf.j2`, requires only the two image variants
to change, then restores the file and requires all original hashes.

On an AMD64 Docker host, run the separate installed-service test:

```sh
python3 tools/bazel/ci/radv_loaded.py --artifacts artifacts/radv
```

It uses the actual installed configuration engine, startup scripts,
supervisor and radvd. Private Redis supplies ToR/EPMS/MgmtTsToR, non-ToR and
IPv4-only template cases. An isolated veth pair tests the VLAN-readiness
gate and captures a real IPv6 advertisement, checking its prefix, managed
flag, autonomous flag, MTU, lifetimes and hop limit. The probe has no host
network or host services; network capabilities apply only inside its own
container. Its Redis test helper is not installed in either image.

The shared Bazel OCI workflow uploads the exact built archives and evidence,
then loads those same archives in a separate job. Full VS installer/boot
acceptance and physical network testing remain separate scopes. Local warm
restore checks do not by themselves prove cross-machine source reproducibility.

The shared build-infra APT lock supplies the pinned radvd runtime and the
`sysmgr_debug_debian` tool selection. Update external packages in build-infra
and review the package controls and installed files.
For script changes, edit the original source file; `config/BUILD.bazel`
declares its installed path and mode directly. Keep this image's new scope
limited to AMD64; inherited architecture checks remain unchanged.

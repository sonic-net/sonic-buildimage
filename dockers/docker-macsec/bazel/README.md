# Native AMD64 MACsec image

This composition keeps the selected Trixie Dockerfile contract. The shared
`docker-swss-layer-trixie` parent already supplies macsecmgrd, Common, OpenSSL,
all five source-built libnl runtime packages and the supervisor utilities.
The child consumes WPA's public `wpasupplicant_pkg` TAR and matching symbols;
it does not rebuild those shared libraries or create Debian packages.

The six Dockerfile-owned supervisor, WPA and CLI files retain their exact source
bytes and 0644/root installation modes. The installed entrypoint remains
`/usr/local/bin/supervisord`, with rsyslogd preceding macsecmgrd. Make's original
prerequisites, service metadata, host CLI hooks and volume flags remain in place;
the Bazel handoff applies only to native AMD64 VS/Trixie with `INCLUDE_FIPS=y`
and the ordinary `dbg` marker, without ASAN, cross compilation or QEMU.

The runtime, debug and private Redis inputs use build-infra's single production
APT lock; this image carries no separate lock or resolver.
Runtime roots are libdbus-1-3, libpcsclite1, libreadline8t64, libtinfo6, lsb-base
and adduser. Common, native libnl and OpenSSL continue to come from their source
owners. The checked APT selector retains matching parent packages and verifies
file conflicts. The source metadata records only the actual WPA package; it
does not invent records for unrelated inherited artifacts.

The installed fixture uses a private Redis instance with both the shipped Unix
socket and a loopback TCP listener, a private veth pair, the actual macsecmgrd
executable and its actual WPA child. It checks the POST and port readiness gates,
then observes the SONiC driver's real MACSEC_PORT_TABLE request. It deliberately
stops before an orchagent/SAI response. This is a software handoff test, not an
ASIC, encrypted-traffic, FIPS POST or installer-boot result. Both image variants
must also pass exact source-payload checks, complete native loader checks and
selected GDB checks. The Redis process is test-only and is not added to the image.

WPA's native source targets live in [its source PR](https://github.com/securely1g/sonic-wpa-supplicant/pull/1).
The runtime adds its `netdev` group while preserving the parent's accounts.
`SONIC_PACKAGES_LOCAL` still selects the enabled runtime/debug package; the
installer accepts the exported `latest` tag and adds `SONIC_IMAGE_VERSION`.

From a clean, committed checkout on native AMD64/Trixie, run the build and
validation commands with the registries configured in the root `.bazelrc`.

```sh
python3 -B tools/bazel/ci/macsec_image.py build --artifacts artifacts/macsec
python3 -B tools/bazel/ci/macsec_image.py validate --artifacts artifacts/macsec
```

The directory contains both `.gz` archives and `validation.json`.

The shared `bazel-oci.yml` workflow builds the AMD64 archives once, checks their
contents and incremental reuse, then passes those same archives to the installed
service test. Output lookup uses each build's recorded execution root, and
incremental receipts retain action/cache counters when Bazel reports them.
Both local and CI builds resolve the WPA and build-infra modules from the SONiC
registry on `main`. The build also uses the Bazel Central Registry.

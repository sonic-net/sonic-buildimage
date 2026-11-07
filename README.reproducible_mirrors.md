# Reproducible Debian mirrors

SONiC builds can use local Debian mirrors by passing mirror URLs during
configuration and build. This is useful for reproducible builds and for sites
that keep an internal package archive.

## Configure and build with the same mirrors

Pass both the normal Debian mirror and the security mirror when generating the
build environment, then keep using the same values for the actual build:

```bash
make configure \
  MIRROR_URLS=http://mirror.example/debian \
  MIRROR_SECURITY_URLS=http://mirror.example/debian-security

make target/sonic-vs.img.gz \
  MIRROR_URLS=http://mirror.example/debian \
  MIRROR_SECURITY_URLS=http://mirror.example/debian-security
```

`scripts/build_mirror_config.sh` renders these values into the generated slave
`sources.list.<arch>` files. The slave image tag includes those generated source
lists, so changing the mirror URLs regenerates the relevant slave context
instead of silently reusing one built from different Debian endpoints.

For old-commit reproducibility, the mirror URL alone is not enough: the mirror
must expose the package versions expected by that commit. Use a frozen internal
snapshot, an accumulating package archive, or SONiC's snapshot flow
(`MIRROR_SNAPSHOT=y` with the matching mirror timestamps) rather than pointing
old commits at a moving current Debian mirror.

## Aptly filtered mirrors need source packages

The slave Dockerfiles run `apt-get build-dep` for Debian source package names
such as:

- `openssh`
- `linux`
- `isc-dhcp`

The generated SONiC `sources.list.<arch>` files include both `deb` and `deb-src`
entries, so a local mirror must publish a usable Debian `Sources` index. When
using Aptly with filtered mirrors, do all of the following:

1. Mirror or import source packages as well as binary packages.
2. Publish the repository with the `source` architecture, for example
   `-architectures="amd64,source"`.
3. Include source package names needed by `apt-get build-dep` in the filter, not
   only binary package names. For example, binaries such as `isc-dhcp-client`
   and `isc-dhcp-relay` come from the `isc-dhcp` source package.

If the mirror has binary packages but no matching source package index, slave
image builds can fail with errors like:

```text
E: Unable to find a source package for isc-dhcp
```

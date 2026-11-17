# Optional Debian source downloads

Monit and rasdaemon normally clone their existing Debian Git repositories at
the pinned tag or commit. Set `PREFER_DEBIAN_REPO=y` to build the same declared
package versions from their checksum-pinned Debian source archives instead:

```sh
make target/sonic-vs.bin PREFER_DEBIAN_REPO=y
```

The option also works with a direct `make -f slave.mk` build. Its default is
`n`; the outer slave wrapper forwards it and package submakes inherit it.
The selected value participates in both packages' Make cache keys.

Monit 5.34.3-1 uses `deb.debian.org`; rasdaemon 0.6.8-1 uses its dated
`snapshot.debian.org` source release. All three files for each package are
checked against their pinned SHA256 values before extraction. Update these
pins together with the package version. A download, checksum, extraction or
build failure stops the selected path; there is no automatic source fallback.

Both paths preserve the existing SONiC patches, Debian packaging and runtime
and debug package outputs. Archive extraction leaves Debian quilt patches
unapplied so SONiC's patches still run first. This option does not select
binary packages or change the source selection for other components.

Native AMD64 package builds validate the archive path. Monit's original cross
patch has stale context for the published 5.34.3-1 Debian rules; cross-builds
using that original patch need the context refresh tracked separately in
[PR #84](https://github.com/securely1g/sonic-buildimage/pull/84). Native AMD64
validation does not establish an ARM or cross-compilation result.

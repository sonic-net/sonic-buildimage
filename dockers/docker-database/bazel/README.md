# AMD64 database source image

The supported profile is native AMD64 Debian Trixie VS, with FIPS enabled and
ASAN/cross/QEMU modes disabled. The runtime image inherits the source-built
config-engine parent, consumes DASH API and its Protobuf runtime directly from
their public source TAR targets, and adds locked Debian Redis and the locked
Click wheel. Common and sonic-db-cli are inherited once. The image source graph
produces runtime/debug TARs and image archives, with no DEB output.

The debug image extends the exact runtime image. It carries matching source
symbols, locked Debian debugging tools and the source-built FIPS OpenSSH client.
Runtime and debug both carry Make's selected image tag and the component versions
for libswsscommon and sonic-supervisord-utilities-rs.
Make selects component labels from the registered dependency metadata without building the DEBs.
The installed package database records `libdashapi` and `libprotobuf32t64`
from their source owners. Each record identifies the selected owner revision
and the exact runtime TAR installed alongside the locked Debian packages.

The original database scripts and five templates are installed unchanged.
Upstream expand_template applies the original Dockerfile's Redis settings to
the pinned package configuration; CI compares its bytes with that exact sed
recipe. Redis account identities are inherited. Directory/configuration modes
and the configuration statoverride follow the real Debian postinst scripts.
The existing supervisor entrypoint starts services; no image-build action starts
host services or executes online apt.

Make keeps image names, host redis-cli, mounts, prerequisites and selection flags.
Direct requests for an existing runtime or debug archive re-enter the worker so a
change of producer takes effect. The supported producer uses the common Bazel archive bridge. Its event-database
selection is an atomic declared environment file: unchanged bytes retain their
mtime, and invalid input cannot replace the previous generation. Other profiles
retain their existing Make producer.

The shared `.github/workflows/bazel-oci.yml` builds and validates the Database
images in its native AMD64 archive job, reusing checkout, Bazel setup, source
preparation and caches. A dependent job loads the archives and tests the installed
services. Its receipt records the PR head, tested merge revision and base. The
consumer checks those revisions and both archive digests before loading either
image. These jobs run on pull requests and master pushes; dedicated source
worker dispatches keep their existing scope.

Build and post-build validation are separate CI steps. The image source graph is
checked for DEB outputs and packaging commands before execution. The normal
regression suite also runs existing packaging tests, which may produce temporary
sample DEBs; it has no branch-specific skips. Content checks
cover parent/source bytes, import records, permissions, debug correspondence,
platform, manifest labels and archive tags. Loaded probes use network-isolated
containers and private writable state, run the real entrypoint for single and
multi-instance configurations, verify 100 Redis databases and source sonic-db-cli
writes, and exercise the installed DASH CLI through its native library. Template
checks cover conditional event DBs, multi-ASIC global includes and chassis
selection. They preserve legacy optional BMP instance definitions without
expanding the migration scope to the BMP container.

No-op and template edit/restore checks verify unchanged parent/source TARs and
exact restored archives. Cross-machine image reproducibility remains dependent
on the separately tracked foreign-source path fixes. Full VS installer, hardware,
and chassis/namespace network acceptance are separate stages.

The production build-infra APT lock supplies the Redis runtime and debug tools.
The `mgmt_test_deps` set contains Redis server and tools, and `sysmgr_debug_debian`
contains the matching debug tools. Update reviewed Debian inputs in build-infra.

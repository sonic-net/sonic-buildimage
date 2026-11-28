# Source-built DASH engine image

The native AMD64/Trixie path builds `docker-dash-engine.gz` from the existing
source base and the public PI, BMv2 and nnpy runtime TARs. It installs the original
`start.sh` unchanged. That script raises `eth1` and `eth2`, then starts
`simple_switch_grpc --interface 0@eth1 --interface 1@eth2 --log-console --no-p4`.
The original Dockerfile's Supervisor startup is commented out, so this image
keeps the direct entrypoint. Its explicitly installed net-tools, Supervisor,
rsyslog and dependent-startup packages remain available.

The canonical APT lock retains the PI/BMv2 owners' declared package dependencies,
including their development/compiler packages, and the Python modules imported
by their installed clients. The native nnpy extension is built from upstream
source against the same nanomsg ABI used by BMv2. Its upstream egg metadata and
license are retained; it is not represented as a Debian package.

Make selects the Ubuntu Noble base `p4lang/behavioral-model:latest`. The
[AMD64 Docker version lock](../../../files/build/versions-public/default/versions-docker)
pins it to `sha256:3318da81246996d3a059e0c1fa66fa5559cb30476f8412825703872050fcb6d5`.
The precise PI and BMv2 source revisions in that image still need verification
for the same-source Make comparison.

## Build and verification

Use native Debian Trixie/AMD64 with Bazel 8.5.1. The shared build entry point uses the maintained SONiC registry.

```sh
python3 -B tools/bazel/ci/dash_engine_image.py build --artifacts artifacts/dash-engine
python3 -B tools/bazel/ci/dash_engine_image.py validate --artifacts artifacts/dash-engine
python3 -B tools/bazel/ci/dash_engine_image.py incremental --artifacts artifacts/dash-engine
python3 -B tools/bazel/ci/dash_engine_loaded.py --artifacts artifacts/dash-engine
```

`build` first renders only Make manifests and audits the configured graph for
DEB producers. Normal Make selects the Bazel archive only for VS/AMD64/Trixie,
FIPS and DASH enabled, without ASAN, RPC or emulation. Other profiles retain the
existing Docker path. The debug archive is a separate validation export; this
change adds no debug installer family to the legacy Make configuration.

The static check binds both image stages to their exact parents, source TARs,
APT selections and installed package state. It preserves inherited account rows,
file metadata and runtime ELFs, and verifies every project ELF's matching build
ID, DWARF and debuglink (with the existing sealed SymCrypt contract). PI's
launcher retains matching debug symbols for its three PI utility sources and forwards `main` to the
source-owned `libpicli.so.0`; the existing package verifier checks that declared
provider, and the installed probe requires GDB source/line/type lookup with a
missing-symbol negative control. The debug image adds the source OpenSSH client, matching symbols and full Vim.

The loaded check uses a disposable Docker network namespace and two private
interfaces. It runs the unchanged startup script, verifies stable installed
processes, P4Runtime 1.3.0 controller arbitration and the selected PI owner's
no-pipeline rejection. It also exercises the installed nanomsg event/debugger
clients, native loader closure and positive/negative GDB lookup. These are
software installation checks, not a full VS boot, forwarding or hardware test.

The incremental check changes the tracked startup script, requires both image
archives to change while source-owner outputs and the parent stay identical,
and restores exact baseline archive bytes. That verifies image invalidation and
cache restoration; it does not claim a fresh rebuild of every native producer.

The DASH engine archive job in `bazel-oci.yml` builds both images on the
`bazel-x64, vs` pool with its own scratch directory. It runs in parallel with
the common archive job. The installed-service job downloads those archives on `ubuntu-24.04`
and reports its result to the required AMD64 archive check. Public artifacts
include the runtime APT layer for non-ELF replacement checks and both filter
files used to verify source ownership. The public reports omit command
environments and local cache paths.

## Build inputs

The image selects the published PI, BMv2, nnpy, Common, libnl and FIPS source
modules using the buildimage module configuration. The archive exporter resolves
external outputs against the execution root of the successful build.

The two AMD64 selections `dash_engine_debian` and `dash_engine_debug_debian`
use the shared sonic-build-infra Debian lock. Their root packages and the locked
dependency graph are maintained in that repository's `apt` directory.

# sonic-device-data
Device-specific data for the SONiC project

See the [SONiC Website](https://sonic-net.github.io/SONiC/) for more information about the SONiC project.

## AMD64 VS installer

Build the TAR from the buildimage checkout.

```sh
cd src/sonic-device-data/workspace
bazel build //src/sonic-device-data:vs_runtime_tar
```

The TAR is at `bazel-bin/src/sonic-device-data/vs_runtime_tar.tar` in the
component workspace. The AMD64 VS installer uses the same target and sources.
The workspace reads the owner BUILD file and device tree directly from the
checkout. Its dependency graph covers the data generator and archive tools,
so a fresh runner can restore the TAR without resolving unrelated image
toolchains. The TAR contains device files, virtual HwSKU profiles and package
documentation. Bazel runs all five Make configuration checks before archiving.

`rules_gzip` builds the Debian changelog with pigz 2.8 and `--no-name`,
which omits the input filename and timestamp. The public musl toolchain
builds pigz and its dependencies from their pinned sources into a static
AMD64 executable. Bazel verifies the BCR source checksums when it fetches
from the original locations or the configured mirrors.
[Compression source](https://registry.bazel.build/modules/pigz/2.8.bcr.2)
and [compiler](https://registry.bazel.build/modules/toolchains_musl/0.2.0).

When several physical platforms define the same HwSKU, the virtual platform
needs one source for its profile. The Bazel target uses lexical path order,
with the choices in `prepare_vs.py` preserving the installed profiles from the
[reference VS installer](https://github.com/securely1g/sonic-buildimage/pull/251).
Installed files use Git executable modes; stage an executable-bit change with
`git add` before building a modified checkout.

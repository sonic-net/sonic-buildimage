# P4RT builds against Common's library and headers

P4RT keeps the pinned `sonic-pins` checkout and **Bazel 6.4.0**. It consumes
Common as a prebuilt SDK: a shared library and its matching development headers.
Common's source checkout builds that SDK with **Bazel 8.5.1**. P4RT does not load
Common's Bazel rules or compile Common's C++ sources.

```text
src/sonic-swss-common                       src/sonic-p4rt/sonic-pins
Bazel 8.5.1                                Bazel 6.4.0
//dist:libswsscommon_sdk                     @sonic_swss_common//:libswsscommon
         |                                          ^
         +--> .common-sdk/ --------------------------+
              lib/libswsscommon.so.0
              include/swss/*.h, *.hpp, c-api/*.h
              include/swss/cfg_schema.h
```

`tools/bazel/p4rt_common_sdk.py` builds `//dist:libswsscommon_sdk` with YANG
enabled, then creates a small Bazel 6 repository using `cc_import`. Both labels
used by P4RT, `:common` and `:libswsscommon`, refer to that imported library.
The generated `cfg_schema.h` comes from the same Common build as the library;
the Makefile no longer copies a schema header from `/usr/include/swss` into
Common's source tree.

Every P4RT Make build invokes the producer again, letting Bazel check changed
sources, headers, generated inputs and options. Cached Bazel actions can still
be reused. The helper replaces the SDK only after a successful build and checks
the library's architecture. Removed headers do not survive from an older SDK.
A failed producer stops the build; it never falls back to an installed or older
Common library. `sdk-build.json` records the source commit, producer version,
architecture, options and archive checksum.

## Native AMD64 and ARM64

Build inside the matching Trixie sonic-slave environment. ARM64 selects Common's
`--config=aarch64`; AMD64 uses its native configuration. The handoff does not
enable cross-compilation or change P4RT's existing Debian packaging rules.

The SDK contains Common's own files. P4RT continues to use the native external
development/runtime packages supplied by sonic-slave: hiredis, libnl3, JSON
headers, Boost, ZeroMQ, UUID and libyang. The installed runtime dependencies must
match Common's selected versions and shared-library names. CI prepares these
dependencies before linking and running the P4RT tests; copying Common's `.so`
alone is not a complete runtime environment.

The Common producer has a private output base next to `.common-sdk` and shuts
down its server after preparing the SDK. P4RT has its own workspace/server.
Both can reuse `/bazel_cache/repository_cache` and `/bazel_cache/disk_cache`
through the existing slave configuration. `BAZEL_CACHE` can select another
mounted cache directory for a direct component build; it never selects a
shared `output_user_root`.

## Validate without producing Debian packages

Prepare the SDK from the repository root:

```sh
python3 tools/bazel/p4rt_common_sdk.py \
  --common-source src/sonic-swss-common \
  --output-dir src/sonic-p4rt/.common-sdk
```

After installing the native dependencies described above, run the same
validation as CI from the repository root:

```sh
python3 tools/bazel/ci/p4rt_common_sdk.py \
  --source src/sonic-p4rt/sonic-pins --sdk src/sonic-p4rt/.common-sdk \
  --output-base src/sonic-p4rt/.bazel-p4rt-output \
  --output artifacts/p4rt --arch amd64
```

Use Bazelisk for automatic selection of the two pinned Bazel versions. The SDK
helper verifies the actual producer version, and the Makefile verifies the
consumer version. The normal P4RT Make packaging target still builds Debian
packages; it is not the validation command above.

The `P4RT with the Common SDK` workflow runs natively on AMD64 and ARM64.
It builds the actual `//p4rt_app:p4rt` executable and runs every non-manual
test from `tests(//p4rt_app/...)` (58 tests at the pinned revision). Manual
tests, such as sonic-pins' x86_64-only `runtime_package_test` that needs gdb,
run in the owner repository's CI instead.
The action check rejects Common compilation or Debian package outputs under
Bazel 6. The executable check verifies its architecture, library name, startup,
and that the loaded Common library has the SDK's checksum. Test results are
executed again instead of accepted from Bazel's test cache. Set `--arch arm64`
on a native ARM64 host.

The helper regression tests run with:

```sh
python3 -B -m unittest discover -s tools/bazel/tests -p p4rt_common_sdk_test.py -v
```

## P4RT packages and debug validation

The existing Make recipe builds `//p4rt_app:p4rt_deb` and
`//p4rt_app:p4rt_dbg_deb` from the pinned `sonic-pins` source. Legacy Docker
builds install those DEBs. `BUILD_WITH_BAZEL_WHEN_AVAILABLE=y` selects Bazel
only for registered OCI images; it does not change P4RT's package format or
inject source TARs into its Dockerfile.

The nested Bazel build and tests fetch Boost's pinned XZ 5.4.6 source archive
from the XZ release, with Fedora's lookaside cache as a fallback. Both URLs
use the SHA-256 from the pinned `rules_boost` repository rule.

For example, the full VS workflow requests both
`target/debs/trixie/sonic-p4rt_0.0.1_amd64.deb` and
`target/debs/trixie/sonic-p4rt-dbgsym_0.0.1_amd64.deb`, then verifies their
split debug symbols with `tools/bazel/ci/verify_p4rt_debug.py` using the paired
`--runtime-deb` and `--debug-deb` options.

The verifier also accepts existing source-owner TAR outputs with paired
`--runtime-tar` and `--debug-tar` options. This standalone validation support
does not register those outputs with Make or feed them into legacy Docker.
Both interfaces require `--source-root` and `--output`.

`tools/bazel/tests/p4rt_tar_test.py` checks standalone archive validation and
legacy DEB selection. `p4rt_debug_test.py` compiles native ELF/DWP fixtures and
checks isolated GDB lookup, including invalid and unrelated symbols. These
fixture checks do not establish production P4RT compilation, image assembly or
forwarding for a given source revision and architecture.

# Bazel Python library and wheel

`//src/sonic-py-common:sonic_py_common` exposes the existing Python helpers to
config-engine and other Bazel consumers. It uses Common's source-built native
Python binding and pinned external Python dependencies. The complete package
also includes the DB dump/load module and the generated `sonic_grpc.gnoi`
modules enabled by `setup.py` for Python 3.9+ on AMD64 and ARM64.

`//src/sonic-py-common:sonic_py_common_wheel` creates
`sonic_py_common-1.0-py3-none-any.whl`. It preserves the package and dependency
metadata, license, generated gRPC modules, and the `sonic-db-load` and
`sonic-db-dump` entry points. The DB helpers use SONiC's patched
`redis-dump-load` source through `@redis_dump_load//:redisdl`. The protobuf
generation action declares its source inputs and pinned generator; it does not write generated files into the checkout.

Ordinary source additions in `sonic_py_common` are picked up by the source glob.
Keep installed package declarations aligned with `setup.py`. Declare a new
third-party import in `BUILD.bazel` and this directory's `requirements.in`.
Config-engine includes that file and resolves both components into one shared
`src/sonic-config-engine/requirements_lock.txt`; both use `@config_engine_pip`
for pip dependencies. Production
`redis-dump-load` uses the registry module; its pip source pin is retained only
for the legacy SWSSSDK test dependency.
For example, change the `natsort` pin here, then
[regenerate the shared lock](../sonic-config-engine/README.bazel.md#python-dependencies).
SONiC libraries depend on targets maintained by their owning repositories.

## Validation

From the repository root, use ordinary Bazel with the maintained
[registry configuration](../../tools/bazel/README.md#registry-selection):

```sh
bazel build //src/sonic-py-common:sonic_py_common_wheel
bazel test \
  //tools/bazel/tests:sonic_py_common_full_test \
  //tools/bazel/tests:sonic_python_wheels_test \
  //tools/bazel/tests:sonic_py_common_test
```

Use native Debian Trixie with Python 3.13. The root defaults select AMD64. On
native ARM64, select both the target and execution toolchains:

```sh
bazel test --platforms=@sonic_build_infra//platforms:aarch64_trixie \
  --host_platform=@sonic_build_infra//platforms:aarch64_trixie \
  //tools/bazel/tests:sonic_py_common_full_test \
  //tools/bazel/tests:sonic_python_wheels_test \
  //tools/bazel/tests:sonic_py_common_test
```

The full source suite discovers the complete `tests/` tree, including DB and
gRPC tests and the existing conftest setup. Its runner supplies writable copies
of the declared source and fixture files and preserves the suite's existing
explicit skips. Source tests may use their existing mocks for Redis and platform
services; those tests establish module behavior, not live service health.

The separate native-dependency check imports the package helpers and exercises
the real Common extension through SWIG. Its existing interface, BGP and EEPROM
cases stay selected. The original `//src/sonic-py-common:sonic_py_common_test`
label remains a compatibility suite for this focused check.

The joint installed-wheel check builds and installs both this package and
config-engine into a temporary installation. It checks imports from the installed
module paths, generated gRPC exports, package metadata and entry points, then
runs the installed DB commands and cfggen with their declared dependencies.
It validates the produced wheel separately from the source test environment.

The native AMD64 and ARM64 archive jobs in
[CI](../../.github/workflows/bazel-swss-oci.yml) run these suites. Their
`container-archive-amd64` and `container-archive-arm64` artifacts retain both
wheels, hashes, source and installed-wheel test logs/XML, collected test
inventories, the installation receipt, and revision evidence. Missing required
outputs or mismatched installed wheel hashes fail artifact collection.

These targets cover the Python 3.13 AMD64/ARM64 package contract. ARMHF's
conditional gRPC exclusion and older Python versions are outside this matrix.
Runtime calls to Redis, platform files, host commands and device services keep
their existing requirements. This work creates no Debian packages or additional
package installation in the existing Make-built container base.
